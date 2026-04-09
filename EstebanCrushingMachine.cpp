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

// SendInput helper function
void sendKeyState(DWORD key, bool down, std::unordered_map<DWORD, bool> &simulatedKeys){
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

// Check the state of keys for a desired hotkey and send (un)presses based on it
void checkState(bool down, DWORD targetKey, DWORD key, std::vector<DWORD> reqKeys, std::unordered_map<DWORD, bool> &simulatedKeys){
    std::cout << "checkState() called\n";
    
    if(down){
        std::cout << "+ Down state detected\t";
        if(!simulatedKeys[targetKey]){
            std::cout << "+ Key not already simulated\t";
            sendKeyState(targetKey, down, simulatedKeys);
        }
    } else {
        std::cout << "+ Down state not detected\t";
        for(DWORD reqKey : reqKeys){
            if(key == reqKey){
                std::cout << "+ " << reqKey << " unpress detected\t";
                sendKeyState(targetKey, down, simulatedKeys);
                break;
            }
        }
    }

    std::cout << "checkState() end\n";
}

// Overload of checkState() to accept a vector of target keys
void checkState(bool down, std::vector<DWORD> targetKeys, DWORD key, std::vector<DWORD> reqKeys, std::unordered_map<DWORD, bool> &simulatedKeys){
    std::cout << "checkState() (overload) called\n";
    
    if(down){
        std::cout << "+ Down state detected\t";
        if(!simulatedKeys[targetKeys.back()]){ // Check if the last key in the list of target keys is being simulated. May or may not change this.
            std::cout << "+ Key not already simulated\t";
            for(DWORD targetKey : targetKeys){
                sendKeyState(targetKey, down, simulatedKeys);
            }
        }
    } else {
        std::cout << "+ Down state not detected\t";
        for(DWORD reqKey : reqKeys){
            if(key == reqKey){
                std::cout << "+ " << reqKey << " unpress detected\t";
                for(DWORD targetKey : targetKeys){
                    sendKeyState(targetKey, down, simulatedKeys);
                }
                break;
            }
        }
    }
    
    std::cout << "checkState() (overload) end\n";
}

// Check key combos for any desired combinations
void checkCombo(std::unordered_map<DWORD, bool> &keysPressed, DWORD key, bool down, std::unordered_map<DWORD, bool> &simulatedKeys){
    std::cout << "checkCombo() called\n";
    
    int num{};
    for(auto key : keysPressed){
        if(key.second){
            ++num;
        };
    }
    std::cout << "Keys pressed before: " << num << "\n";

    if(down) keysPressed[key] = true;

    num = 0;
    for(auto key : keysPressed){
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
    */

    if(keysPressed[VK_LSHIFT] || keysPressed[VK_RSHIFT]){ // Shift keys
        std::cout << "Shift detected\t";
        if(keysPressed['W']){ // Volume up
            std::cout << "+ W detected\t";
            checkState(down, VK_VOLUME_UP, key, {'W', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
        }
        std::cout << "\n";
        if(keysPressed['A']){ // Media prev
            checkState(down, VK_MEDIA_PREV_TRACK, key, {'A', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
        }
        if(keysPressed['R']){ // Volume down
            checkState(down, VK_VOLUME_DOWN, key, {'R', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
        }
        if(keysPressed['S']){ // Media next
            checkState(down, VK_MEDIA_NEXT_TRACK, key, {'S', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
        }
        if(keysPressed['F']){ // Media play/pause
            checkState(down, VK_MEDIA_PLAY_PAUSE, key, {'F', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
        }
        if(keysPressed['Q']){ // Volume mute
            checkState(down, VK_VOLUME_MUTE, key, {'Q', VK_LSHIFT, VK_RSHIFT}, simulatedKeys);
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
        if(keysPressed['U']){ // Up
            checkState(down, VK_UP, key, {'U'}, simulatedKeys);
        }
        if(keysPressed['N']){ // Left
            checkState(down, VK_LEFT, key, {'N'}, simulatedKeys);
        }
        if(keysPressed['E']){ // Down
            checkState(down, VK_DOWN, key, {'E'}, simulatedKeys);
        }
        if(keysPressed['I']){ // Right
            checkState(down, VK_RIGHT, key, {'I'}, simulatedKeys);
        }
        if(keysPressed['L']){ // Home
            checkState(down, VK_HOME, key, {'L'}, simulatedKeys);
        }
        if(keysPressed['Y']){ // End
            checkState(down, VK_END, key, {'Y'}, simulatedKeys);
        }
        if(keysPressed[VK_OEM_1]){ // Page Up | VK_OEM_1 = Semicolon/Colon key
            checkState(down, VK_PRIOR, key, {VK_OEM_1}, simulatedKeys);
        }
        if(keysPressed['O']){ // Page Down
            checkState(down, VK_NEXT, key, {'O'}, simulatedKeys);
        }
        if(keysPressed[VK_OEM_7]){ // Insert | VK_OEM_7 = Apostrophe/Double Quotation Mark key
            checkState(down, VK_INSERT, key, {VK_OEM_7}, simulatedKeys);
        }
        if(keysPressed['H']){ // Print Screen
            checkState(down, VK_SNAPSHOT, key, {'H'}, simulatedKeys);
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
        if(keysPressed['W']){ // Up
            checkState(down, VK_UP, key, {'W'}, simulatedKeys);
        }
        if(keysPressed['A']){ // Left
            checkState(down, VK_LEFT, key, {'A'}, simulatedKeys);
        }
        if(keysPressed['R']){ // Down
            checkState(down, VK_DOWN, key, {'R'}, simulatedKeys);
        }
        if(keysPressed['S']){ // Right
            checkState(down, VK_RIGHT, key, {'S'}, simulatedKeys);
        }
        if(keysPressed['F']){ // Enter
            checkState(down, VK_RETURN, key, {'F'}, simulatedKeys);
        }
        if(keysPressed['Q']){ // End
            checkState(down, VK_END, key, {'Q'}, simulatedKeys);
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
        if(keysPressed['1']){ // F1
            checkState(down, VK_F1, key, {'1'}, simulatedKeys);
        }
        if(keysPressed['2']){ // F2
            checkState(down, VK_F2, key, {'2'}, simulatedKeys);
        }
        if(keysPressed['3']){ // F3
            checkState(down, VK_F3, key, {'3'}, simulatedKeys);
        }
        if(keysPressed['4']){ // F4
            checkState(down, VK_F4, key, {'4'}, simulatedKeys);
        }
        if(keysPressed['5']){ // F5
            checkState(down, VK_F5, key, {'5'}, simulatedKeys);
        }
        if(keysPressed['6']){ // F6
            checkState(down, VK_F6, key, {'6'}, simulatedKeys);
        }
        if(keysPressed['7']){ // F7
            checkState(down, VK_F7, key, {'7'}, simulatedKeys);
        }
        if(keysPressed['8']){ // F8
            checkState(down, VK_F8, key, {'8'}, simulatedKeys);
        }
        if(keysPressed['9']){ // F9
            checkState(down, VK_F9, key, {'9'}, simulatedKeys);
        }
        if(keysPressed['0']){ // F10
            checkState(down, VK_F10, key, {'0'}, simulatedKeys);
        }
        if(keysPressed[VK_OEM_MINUS]){ // F11 | VK_OEM_MINUS = Dash/Underscore key
            checkState(down, VK_F11, key, {VK_OEM_MINUS}, simulatedKeys);
        }
        if(keysPressed[VK_OEM_PLUS]){ // F12 | VK_OEM_PLUS = Equals/Plus key
            checkState(down, VK_F12, key, {VK_OEM_PLUS}, simulatedKeys);
        }
    }

    if(!down) keysPressed[key] = false;
    std::cout << "checkCombo() end\n";
}


void unpressSimulatedKeys(std::unordered_map<DWORD, bool> &simulatedKeys){
    std::cout << "unpressSimulatedKeys() called\n";
    for(std::pair<DWORD, bool> key : simulatedKeys){
        if(key.second){
            sendKeyState(key.first, false, simulatedKeys);
        }
    }
    std::cout << "unpressSimulatedKeys() end\n";
}


// Function to handle hook events
LRESULT CALLBACK hotkeys(int ncode, WPARAM wparam, LPARAM lparam){
    // Static lifetime variables for tracking between functions
    static bool capsPressed; 
    static std::unordered_map<DWORD, bool> pressedKeys;
    static std::unordered_map<DWORD, bool> simulatedKeys;

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
                unpressSimulatedKeys(simulatedKeys);
                pressedKeys.clear();
                simulatedKeys.clear();
                capsPressed = false;
                return 1;
            }
        }

        // Caps modifier
        if(capsPressed && (wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN)){
            std::cout << "Key " << input->vkCode << " pressed with caps down\n";
            checkCombo(pressedKeys, input->vkCode, true, simulatedKeys);
            return 1;
        }
        if(capsPressed && (wparam == WM_KEYUP || wparam == WM_SYSKEYUP)){
            std::cout << "Key " << input->vkCode << " unpressed with caps down\n";
            checkCombo(pressedKeys, input->vkCode, false, simulatedKeys);
            return 1;
        }

        // M
    
    }

    return CallNextHookEx(NULL, ncode, wparam, lparam);
}

// Class to manage the hook with the constructor and destructor
class Keyboard{
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
