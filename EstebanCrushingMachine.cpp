#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <vector>


// Print windows-based errors if needed
void displayError(DWORD dw){
    LPTSTR msgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        0,
        (LPTSTR)&msgBuf,
        0, NULL
    );
    
    std::cerr << msgBuf << "\n";
    LocalFree(msgBuf); // Free the buffer from memory
}


// Class to manage the hook with the constructor and destructor
class Keyboard{
    // Static lifetime variables for tracking between functions
    static inline bool capsPressed; 
    static inline std::unordered_map<DWORD, bool> pressedKeys;
    static inline std::unordered_map<DWORD, bool> simulatedKeys;

    // SendInput helper function
    static void sendKeyState(DWORD key, bool down){
        std::cout << "sendKeyState() called\n";

        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = static_cast<WORD>(key);
        input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

        UINT sent {SendInput(1, &input, sizeof(INPUT))};
        if(sent == 1){
            if(down){
                simulatedKeys[key] = true;
            } else {
                simulatedKeys[key] = false;
            }
        } else {
            std::cout << "Error on SendInput\n";
            displayError(GetLastError());
        }
        std::cout << "sendKeyState() end\n";
    }

    // Overload of sendKeyState to accepted a vector of keys
    static void sendKeyState(std::vector<DWORD> &keys, bool down){
        std::cout << "sendKeyState() (overload) called\n";

        std::vector<INPUT> inputs;
        for(DWORD key : keys){
            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(key);
            input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

            inputs.push_back(input);
        }

        UINT sent {SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT))};
        if(sent == inputs.size()){
            if(down){
                for(DWORD key : keys){
                    simulatedKeys[key] = true;
                }
            } else {
                for(DWORD key : keys){
                    simulatedKeys[key] = false;
                }
            }
        } else {
            std::cout << "Error on SendInput\n";
            displayError(GetLastError());
        }
        std::cout << "sendKeyState() (overload) end\n";
    }

    // Check the state of keys for a desired hotkey and send (un)presses based on it
    static void checkState(bool down, DWORD targetKey, DWORD key, std::vector<DWORD> reqKeys){
        std::cout << "checkState() called\n";
        
        if(down){
            std::cout << "+ Down state detected\t";
            if(!simulatedKeys[targetKey]){
                std::cout << "+ Key not already simulated\t";
                sendKeyState(targetKey, down);
            }
        } else {
            std::cout << "+ Down state not detected\t";
            for(DWORD reqKey : reqKeys){
                if(key == reqKey){
                    std::cout << "+ " << reqKey << " unpress detected\t";
                    sendKeyState(targetKey, down);
                    break;
                }
            }
        }

        std::cout << "checkState() end\n";
    }

    // Overload of checkState() to accept a vector of target keys
    static void checkState(bool down, std::vector<DWORD> targetKeys, DWORD key, std::vector<DWORD> reqKeys){
        std::cout << "checkState() (overload) called\n";
        
        if(down){
            std::cout << "+ Down state detected\t";
            if(!simulatedKeys[targetKeys.back()]){ // Check if the last key in the list of target keys is being simulated. May or may not change this.
                std::cout << "+ Key not already simulated\t";
                sendKeyState(targetKeys, down);
            }
        } else {
            std::cout << "+ Down state not detected\t";
            for(DWORD reqKey : reqKeys){
                if(key == reqKey){
                    std::cout << "+ " << reqKey << " unpress detected\t";
                    sendKeyState(targetKeys, down);
                    
                    break;
                }
            }
        }
        
        std::cout << "checkState() (overload) end\n";
    }

    // Check key combos for any desired combinations
    static void checkCombo(DWORD key, bool down){
        std::cout << "checkCombo() called\n";
        
        int num{};
        for(auto key : pressedKeys){
            if(key.second){
                ++num;
            };
        }
        std::cout << "Keys pressed before: " << num << "\n";

        if(down) pressedKeys[key] = true;

        num = 0;
        for(auto key : pressedKeys){
            if(key.second){
                ++num;
            };
        }
        std::cout << "Keys pressed after: " << num << "\n";
        

        /*
        ; left media control layer while holding shift
        +w::Volume_Up
        +a::Media_Prev
        +r::Volume_Down
        +s::Media_Next
        +f::Media_Play_Pause
        +q::Volume_Mute
        +Esc::~
        */

        if(pressedKeys[VK_LSHIFT] || pressedKeys[VK_RSHIFT]){ // Shift keys 
            std::cout << "Shift detected\t";
            if(pressedKeys['W']){ // Volume up
                std::cout << "+ W detected\t";
                checkState(down, VK_VOLUME_UP, key, {'W', VK_LSHIFT, VK_RSHIFT});
            }
            std::cout << "\n";
            if(pressedKeys['A']){ // Media prev
                checkState(down, VK_MEDIA_PREV_TRACK, key, {'A', VK_LSHIFT, VK_RSHIFT});
            }
            if(pressedKeys['R']){ // Volume down
                checkState(down, VK_VOLUME_DOWN, key, {'R', VK_LSHIFT, VK_RSHIFT});
            }
            if(pressedKeys['S']){ // Media next
                checkState(down, VK_MEDIA_NEXT_TRACK, key, {'S', VK_LSHIFT, VK_RSHIFT});
            }
            if(pressedKeys['F']){ // Media play/pause
                checkState(down, VK_MEDIA_PLAY_PAUSE, key, {'F', VK_LSHIFT, VK_RSHIFT});
            }
            if(pressedKeys['Q']){ // Volume mute
                checkState(down, VK_VOLUME_MUTE, key, {'Q', VK_LSHIFT, VK_RSHIFT});
            }
            if(pressedKeys[VK_ESCAPE]){ // Tilde ~ | VK_OEM_3 = Tilde/Grave key`
                checkState(down, {VK_LSHIFT, VK_OEM_3}, key, {VK_ESCAPE, VK_LSHIFT, VK_RSHIFT});
            }
        } else { // Non-shift hotkeys
            /*
            ; right cluster
            u::Up
            n::Left
            e::Down
            i::Right
            l::Home
            y::End
            `;::PgUp ; remap semi-colon
            o::PgDn
            '::insert ; remap single quote
            h::PrintScreen
            */
            if(pressedKeys['U']){ // Up
                checkState(down, VK_UP, key, {'U'});
            }
            if(pressedKeys['N']){ // Left
                checkState(down, VK_LEFT, key, {'N'});
            }
            if(pressedKeys['E']){ // Down
                checkState(down, VK_DOWN, key, {'E'});
            }
            if(pressedKeys['I']){ // Right
                checkState(down, VK_RIGHT, key, {'I'});
            }
            if(pressedKeys['L']){ // Home
                checkState(down, VK_HOME, key, {'L'});
            }
            if(pressedKeys['Y']){ // End
                checkState(down, VK_END, key, {'Y'});
            }
            if(pressedKeys[VK_OEM_1]){ // Page Up | VK_OEM_1 = Semicolon/Colon key
                checkState(down, VK_PRIOR, key, {VK_OEM_1});
            }
            if(pressedKeys['O']){ // Page Down
                checkState(down, VK_NEXT, key, {'O'});
            }
            if(pressedKeys[VK_OEM_7]){ // Insert | VK_OEM_7 = Apostrophe/Double Quotation Mark key
                checkState(down, VK_INSERT, key, {VK_OEM_7});
            }
            if(pressedKeys['H']){ // Print Screen
                checkState(down, VK_SNAPSHOT, key, {'H'});
            }
            
            /*
            ; simplified left cluster
            w::Up
            a::Left
            r::Down
            s::Right
            f::Enter
            q::End
            */
            if(pressedKeys['W']){ // Up
                checkState(down, VK_UP, key, {'W'});
            }
            if(pressedKeys['A']){ // Left
                checkState(down, VK_LEFT, key, {'A'});
            }
            if(pressedKeys['R']){ // Down
                checkState(down, VK_DOWN, key, {'R'});
            }
            if(pressedKeys['S']){ // Right
                checkState(down, VK_RIGHT, key, {'S'});
            }
            if(pressedKeys['F']){ // Enter
                checkState(down, VK_RETURN, key, {'F'});
            }
            if(pressedKeys['Q']){ // End
                checkState(down, VK_END, key, {'Q'});
            }

            /*
            ; function keys
            1::F1
            2::F2
            3::F3
            4::F4
            5::F5
            6::F6
            7::F7
            8::F8
            9::F9
            0::F10
            -::F11
            =::F12
            */
            if(pressedKeys['1']){ // F1
                checkState(down, VK_F1, key, {'1'});
            }
            if(pressedKeys['2']){ // F2
                checkState(down, VK_F2, key, {'2'});
            }
            if(pressedKeys['3']){ // F3
                checkState(down, VK_F3, key, {'3'});
            }
            if(pressedKeys['4']){ // F4
                checkState(down, VK_F4, key, {'4'});
            }
            if(pressedKeys['5']){ // F5
                checkState(down, VK_F5, key, {'5'});
            }
            if(pressedKeys['6']){ // F6
                checkState(down, VK_F6, key, {'6'});
            }
            if(pressedKeys['7']){ // F7
                checkState(down, VK_F7, key, {'7'});
            }
            if(pressedKeys['8']){ // F8
                checkState(down, VK_F8, key, {'8'});
            }
            if(pressedKeys['9']){ // F9
                checkState(down, VK_F9, key, {'9'});
            }
            if(pressedKeys['0']){ // F10
                checkState(down, VK_F10, key, {'0'});
            }
            if(pressedKeys[VK_OEM_MINUS]){ // F11 | VK_OEM_MINUS = Dash/Underscore key
                checkState(down, VK_F11, key, {VK_OEM_MINUS});
            }
            if(pressedKeys[VK_OEM_PLUS]){ // F12 | VK_OEM_PLUS = Equals/Plus key
                checkState(down, VK_F12, key, {VK_OEM_PLUS});
            }

            /*
            ; send backtick
            Esc::
                Send, ``
                return
            
            Backspace::Del
            */
            if(pressedKeys[VK_ESCAPE]){ // Grave or backtick ` | VK_OEM_3 = Tilde/Grave key
                checkState(down, VK_OEM_3, key, {VK_ESCAPE});
            }
            if(pressedKeys[VK_BACK]){ // Delete
                checkState(down, VK_DELETE, key, {VK_BACK});
            }
        }

        if(!down) pressedKeys[key] = false;
        std::cout << "checkCombo() end\n";
    }


    static void unpressSimulatedKeys(){
        std::cout << "unpressSimulatedKeys() called\n";
        for(std::pair<DWORD, bool> key : simulatedKeys){
            if(key.second){
                sendKeyState(key.first, false);
            }
        }
        std::cout << "unpressSimulatedKeys() end\n";
    }

    // Function to handle hook events
    static LRESULT CALLBACK hotkeys(int ncode, WPARAM wparam, LPARAM lparam){
        /* If nCode is less than zero, the hook procedure must pass the message 
        to the CallNextHookEx function without further processing and should 
        return the value returned by CallNextHookEx. */
        if(ncode >= 0 && ((wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN) || (wparam == WM_KEYUP || wparam == WM_SYSKEYUP))){

            /* Microsoft documentation says the given LPARAM is a pointer to
            a KBDLLHOOKSTRUCT so we cast it to one */
            KBDLLHOOKSTRUCT* input { (KBDLLHOOKSTRUCT*)lparam };

            // Ignore simulated (injected) keys
            if(input->flags & LLKHF_INJECTED) return CallNextHookEx(NULL, ncode, wparam, lparam);
            
            // Intercept Caps Lock
            if(input->vkCode == VK_CAPITAL){
                if(wparam == WM_KEYDOWN){
                    std::cout << "Caps down\n";
                    capsPressed = true;
                    return 1;
                }
                if(wparam == WM_KEYUP){
                    std::cout << "Caps up\n";
                    unpressSimulatedKeys();
                    pressedKeys.clear();
                    simulatedKeys.clear();
                    capsPressed = false;
                    return 1;
                }
            }

            // Caps modifier
            if(capsPressed && (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN)){
                std::cout << "Key " << input->vkCode << " pressed with caps down\n";
                checkCombo(input->vkCode, true);
                return 1;
            }
            if(capsPressed && (wparam == WM_KEYUP || wparam == WM_SYSKEYUP)){
                std::cout << "Key " << input->vkCode << " unpressed with caps down\n";
                checkCombo(input->vkCode, false);
                return 1;
            }
        }

        return CallNextHookEx(NULL, ncode, wparam, lparam);
    }

    HHOOK hHook{NULL}; // Handle to the hook

    public:
    Keyboard(){ // Constructor will install hook
        hHook = SetWindowsHookEx(WH_KEYBOARD_LL, hotkeys, NULL, 0);

        if(hHook == NULL){
            std::cerr << "Hook failed to install.\n";
            displayError(GetLastError());
        } else {
            std::cout << "Hook installed successfully.\n";
        }
    }
    ~Keyboard(){ // Destructor will remove hook at the end of the object's lifetime
        if(hHook != NULL){
            UnhookWindowsHookEx(hHook);
            std::cout << "Hook uninstalled.\n";
        }
    }
    bool checkHook(){ // Check if the hook is installed
        return hHook != NULL;
    }
};




int main(){
    Keyboard hook;
    if (!hook.checkHook()) return 1; // Exit abnormally on failure to hook keyboard
    
    MSG msg;

    // GetMessage has a possibility of being -1 on error
    BOOL returnCode;
    while((returnCode = GetMessage(&msg, NULL, 0, 0)) != 0){
        if(returnCode == -1){
            std::cerr << "Error on message loop\n";
            displayError(GetLastError());
            return 1; // Exit abnormally on message loop error
        } else {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    return 0;
}
