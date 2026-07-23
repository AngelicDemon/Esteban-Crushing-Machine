#include <iostream>
#include <windows.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

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

    struct Keybind{
        std::vector<DWORD> targetKeys;
        bool shift;
        bool strict;

        bool operator==(const Keybind &X){
            if((this->targetKeys.size() != X.targetKeys.size()) || (this->shift != X.shift)) return false;

            std::vector<DWORD> tempThis{this->targetKeys};
            std::vector<DWORD> tempX{X.targetKeys};
        
            std::sort(tempThis.begin(), tempThis.end());
            std::sort(tempX.begin(), tempX.end());

            return tempThis == tempX;
        }
        bool operator!=(const Keybind &X){
            return !(*this==X);
        }
    };
    static inline std::unordered_multimap<DWORD, Keybind> keybinds;
    static inline std::unordered_multimap<DWORD, Keybind> simulatedKeys;

    // Overload of sendKeyState to accepted a vector of keys
    static void sendKeyState(const std::vector<DWORD> &keys, bool down){
        std::cout << "sendKeyState() (overload) called\n";

        std::vector<INPUT> inputs;
        for(DWORD key : keys){
            INPUT input;
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(key);
            input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
            switch(key){
                case VK_UP:
                case VK_LEFT:
                case VK_DOWN:
                case VK_RIGHT:
                case VK_HOME:
                case VK_END:
                case VK_PRIOR:
                case VK_NEXT:
                    input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
            }
            inputs.push_back(input);
        }

        UINT sent {SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT))};
        if(!(sent == inputs.size())){
            std::cout << "Error on SendInput\n";
            displayError(GetLastError());
        }
        std::cout << "sendKeyState() (overload) end\n";
    }


    // Overload of checkState() to accept a vector of target keys
    static void checkState(bool down, const Keybind &keybind, DWORD key){
        std::cout << "checkState() (overload) called\n";
        
        if(down){
            std::cout << "+ Down state detected\t";
            bool sendKeys{true};

            auto range{simulatedKeys.equal_range(key)};
            for(auto it = range.first; it != range.second; ++it){
                if(it->second == keybind){
                    sendKeys = false;
                }
            }
            if(sendKeys){
                sendKeyState(keybind.targetKeys, down);
                simulatedKeys.insert({key, keybind});
            }
        } else {
            std::cout << "+ Down state not detected\t";
            
            auto range{simulatedKeys.equal_range(key)};
            for(auto it = range.first; it != range.second; ++it){
                if(it->second == keybind) simulatedKeys.erase(it);
            }
            sendKeyState(keybind.targetKeys, down);
        }
        
        std::cout << "checkState() (overload) end\n";
    }

    // Check key combos for any desired combinations
    static int checkCombo(DWORD key, bool down){
        std::cout << "checkCombo() called\n";
        
        // Get shift state.
        bool shiftDown{GetAsyncKeyState(VK_SHIFT) < 0};

        int count{};

        if(shiftDown) std::cout << "Shift reported down\n"; else std::cout << "Shift reported up\n";
        
        // Get a list of keybinds for (un)pressed key
        auto range{keybinds.equal_range(key)};
        for(auto it = range.first; it != range.second; ++it){
            const Keybind& keybind{it->second};

            if(!down || shiftDown == keybind.shift || !keybind.strict){
                checkState(down, keybind, key);
                ++count;
            }
        }

        std::cout << "checkCombo() end\n";
        return count;
    }


    static void unpressSimulatedKeys(DWORD key){
        std::cout << "unpressSimulatedKeys() called\n";

        auto range{simulatedKeys.equal_range(key)};
        for(auto it = range.first; it != range.second; ++it){
            sendKeyState(it->second.targetKeys, false);
        }
        simulatedKeys.erase(key);

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

            DWORD vkCode{input->vkCode};
            
            // Intercept Caps Lock
            if(vkCode == VK_CAPITAL){
                if(wparam == WM_KEYDOWN){
                    std::cout << "Caps down\n";
                    capsPressed = true;
                    return 1;
                }
                if(wparam == WM_KEYUP){
                    std::cout << "Caps up\n";
                    capsPressed = false;
                    return 1;
                }
            }

            // Caps modifier
            if(capsPressed){
                size_t count{keybinds.count(vkCode)};

                if(wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN){
                    std::cout << "Key " << vkCode << " pressed with caps down\n";
                    if(count){
                        return checkCombo(vkCode, true);
                    }
                }
                else if(wparam == WM_KEYUP || wparam == WM_SYSKEYUP){
                    std::cout << "Key " << vkCode << " unpressed with caps down\n";
                    if(count){
                        return checkCombo(vkCode, false);
                    }
                }
            }

            // Unpress keybind without caps logic
            if((wparam == WM_KEYUP || wparam == WM_SYSKEYUP) && simulatedKeys.count(vkCode)){
                unpressSimulatedKeys(input->vkCode);
                return 1;
            }
            if((wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN) && simulatedKeys.count(vkCode)){
                
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
    
    void addKeybind(DWORD reqKey, std::vector<DWORD> tarKeys, bool shift = false, bool strict = true){
        Keybind keybind{tarKeys, shift, strict};
        
        keybinds.insert({reqKey, keybind});
    }
    void addKeybind(DWORD reqKey, DWORD tarKey, bool shift = false, bool strict = true){
        std::vector<DWORD> tempVec{tarKey};
        Keybind keybind{tempVec, shift, strict};

        keybinds.insert({reqKey, keybind});
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
    hook.addKeybind('W', VK_VOLUME_UP, true); // Volume Up
    hook.addKeybind('A', VK_MEDIA_PREV_TRACK, true); // Media Prev
    hook.addKeybind('R', VK_VOLUME_DOWN, true); // Volume Down
    hook.addKeybind('S', VK_MEDIA_NEXT_TRACK, true); // Media Next
    hook.addKeybind('F', VK_MEDIA_PLAY_PAUSE, true); // Media Play/Pause
    hook.addKeybind('Q', VK_VOLUME_MUTE, true); // Volume Mute
    hook.addKeybind(VK_ESCAPE, {VK_LSHIFT, VK_OEM_3}, true); // Tilde ~ | VK_OEM_3 = Tilde/Grave key
    
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
    hook.addKeybind('U', VK_UP, false, false); // Up
    hook.addKeybind('N', VK_LEFT, false, false); // Left
    hook.addKeybind('E', VK_DOWN, false, false); // Down
    hook.addKeybind('I', VK_RIGHT, false, false); // Right
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
