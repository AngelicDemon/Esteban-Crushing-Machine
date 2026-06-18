#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <vector>
#include <set>


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

    struct Keybind{
        std::set<DWORD> requiredKeys;
        std::vector<DWORD> targetKeys;
    };
    static inline std::vector<Keybind> keybinds;

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
        if(pressedKeys[VK_LSHIFT] || pressedKeys[VK_RSHIFT]){ // Shift keys 
            std::cout << "Shift detected\t";
            if(pressedKeys['W']){ // Volume up
                std::cout << "+ W detected\t";
                checkState(down, VK_VOLUME_UP, key, {'W', VK_LSHIFT, VK_RSHIFT});
            }
        }
        */


        // Go through every keybind
        for(Keybind keybind : keybinds){
            bool active{};
            const std::set<DWORD> &reqKeys{keybind.requiredKeys}; // Alias to shorten name
            // size_t keyCount{}; // Tracking number of matching keys

            // If the key is needed in a specific keybind
            if(reqKeys.count(key)){
                // Modifier for both being activated by either shift key
                bool bothShifts{static_cast<bool>(reqKeys.count(VK_SHIFT))};

                // Ignore keybind if shift is held and keybind doesn't require it
                bool shiftHeld{pressedKeys[VK_LSHIFT] || pressedKeys[VK_RSHIFT]};
                bool shiftRequired{bothShifts || reqKeys.count(VK_LSHIFT) || reqKeys.count(VK_RSHIFT)};
                if((shiftRequired && !shiftHeld) || (shiftHeld && !shiftRequired)) continue;


                // Check pressed keys for required keys
                for(DWORD reqKey : reqKeys){
                    if(pressedKeys[reqKey] || (reqKey == VK_SHIFT && shiftHeld)){
                        active = true;
                    } else {
                        active = false;
                    }
                }
            }

            if(active){
                std::cout << "ACTIVE KEYBIND\t";
                for(DWORD key : reqKeys){
                    std::cout << key << "\t";
                }
                if(down) std::cout << "ON"; else std::cout << "OFF";
                std::cout << "\n";
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
    
    void addKeybind(std::vector<DWORD> reqKeys, std::vector<DWORD> tarKeys){
        std::set<DWORD> tempSet(reqKeys.begin(), reqKeys.end());

        Keybind keybind{tempSet, tarKeys};
        keybinds.push_back(keybind);
    }
    void addKeybind(DWORD reqKey, std::vector<DWORD> tarKeys){
        std::set<DWORD> tempSet{reqKey};

        Keybind keybind{tempSet, tarKeys};
        keybinds.push_back(keybind);
    }
    void addKeybind(DWORD reqKey, DWORD tarKey){
        std::set<DWORD> tempSet{reqKey};
        std::vector<DWORD> tempVec{tarKey};

        Keybind keybind{tempSet, tempVec};
        keybinds.push_back(keybind);
    }
    void addKeybind(std::vector<DWORD> reqKeys, DWORD tarKey){
        std::set<DWORD> tempSet(reqKeys.begin(), reqKeys.end());
        std::vector<DWORD> tempVec{tarKey};
        
        Keybind keybind{tempSet, tempVec};
        keybinds.push_back(keybind);
    }
};




int main(){
    Keyboard hook;
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
    hook.addKeybind({VK_SHIFT, 'W'}, VK_VOLUME_UP); // Volume Up
    hook.addKeybind({VK_SHIFT, 'A'}, VK_MEDIA_PREV_TRACK); // Media Prev
    hook.addKeybind({VK_SHIFT, 'R'}, VK_VOLUME_DOWN); // Volume Down
    hook.addKeybind({VK_SHIFT, 'S'}, VK_MEDIA_NEXT_TRACK); // Media Next
    hook.addKeybind({VK_SHIFT, 'F'}, VK_MEDIA_PLAY_PAUSE); // Media Play/Pause
    hook.addKeybind({VK_SHIFT, 'Q'}, VK_VOLUME_MUTE); // Volume Mute
    hook.addKeybind({VK_SHIFT, VK_ESCAPE}, {VK_LSHIFT, VK_OEM_3}); // Tilde ~ | VK_OEM_3 = Tilde/Grave key
    
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
    hook.addKeybind('U', VK_UP); // Up
    hook.addKeybind('N', VK_LEFT); // Left
    hook.addKeybind('E', VK_DOWN); // Down
    hook.addKeybind('I', VK_RIGHT); // Right
    hook.addKeybind('L', VK_HOME); // Home
    hook.addKeybind('Y', VK_END); // End
    hook.addKeybind(VK_OEM_1, VK_PRIOR); // Page Up | VK_OEM_1 = Semicolon/Colon key
    hook.addKeybind('O', VK_NEXT); // Page Down
    hook.addKeybind(VK_OEM_7, VK_INSERT); // Insert | VK_OEM_7 = Apostrophe/Double Quotation Mark key
    hook.addKeybind('H', VK_SNAPSHOT); // Print Screen
    
    /*
    ; simplified left cluster
    w::Up
    a::Left
    r::Down
    s::Right
    f::Enter
    q::End
    */
    hook.addKeybind('W', VK_UP); // Up
    hook.addKeybind('A', VK_LEFT); // Left
    hook.addKeybind('R', VK_DOWN); // Down
    hook.addKeybind('S', VK_RIGHT); // Right
    hook.addKeybind('F', VK_RETURN); // Enter
    hook.addKeybind('Q', VK_END);

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
    hook.addKeybind('1', VK_F1); // F1
    hook.addKeybind('2', VK_F2); // F2
    hook.addKeybind('3', VK_F3); // F3
    hook.addKeybind('4', VK_F4); // F4
    hook.addKeybind('5', VK_F5); // F5
    hook.addKeybind('6', VK_F6); // F6
    hook.addKeybind('7', VK_F7); // F7
    hook.addKeybind('8', VK_F8); // F8
    hook.addKeybind('9', VK_F9); // F9
    hook.addKeybind('0', VK_F10); // F10
    hook.addKeybind(VK_OEM_MINUS, VK_F11); // F11 | VK_OEM_MINUS = Dash/Underscore key
    hook.addKeybind(VK_OEM_PLUS, VK_F12); // F12 | VK_OEM_PLUS = Equals/Plus key
    
    /*
    ; send backtick
    Esc::
        Send, ``
        return
    
    Backspace::Del
    */
    hook.addKeybind(VK_ESCAPE, VK_OEM_3); // Grave or backtick ` | VK_OEM_3 = Tilde/Grave key
    hook.addKeybind(VK_BACK, VK_DELETE); // Delete


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
