#include <iostream>
#include <windows.h>
#include <unordered_map>


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

    if(keysPressed[VK_LSHIFT] || keysPressed[VK_RSHIFT]){
        bool keyIsShift{key == VK_LSHIFT || key == VK_RSHIFT};
        std::cout << "Shift detected\t";
        if(keysPressed['W']){ // Volume up
            std::cout << "+ W detected\t";
            if(down){
                std::cout << "+ Down state detected\t";
                if(!simulatedKeys[VK_VOLUME_UP]){
                    std::cout << "+ Key not already simulated\t";
                    sendKeyState(VK_VOLUME_UP, down, simulatedKeys);
                }
            } else {
                std::cout << "+ Down state not detected\t";
                if(keyIsShift || key == 'W'){
                    std::cout << "+ Shift or W unpress detected\t";
                    sendKeyState(VK_VOLUME_UP, down, simulatedKeys);
                }
            }
        }
        std::cout << "\n";
        if(keysPressed['A']){ // Media prev
            if(down){
                if(!simulatedKeys[VK_MEDIA_PREV_TRACK]){
                    sendKeyState(VK_MEDIA_PREV_TRACK, down, simulatedKeys);
                }
            } else {
                if(keyIsShift || key == 'A'){
                    sendKeyState(VK_MEDIA_PREV_TRACK, down, simulatedKeys);
                }
            }
        }
        if(keysPressed['R']){ // Volume down
            if(down){
                if(!simulatedKeys[VK_VOLUME_DOWN]){
                    sendKeyState(VK_VOLUME_DOWN, down, simulatedKeys);
                }
            } else {
                if(keyIsShift || key == 'R'){
                    sendKeyState(VK_VOLUME_DOWN, down, simulatedKeys);
                }
            }
        }
        if(keysPressed['S']){ // Media next
            if(down){
                if(!simulatedKeys[VK_MEDIA_NEXT_TRACK]){
                    sendKeyState(VK_MEDIA_NEXT_TRACK, down, simulatedKeys);
                }
            } else {
                if(keyIsShift || key == 'S'){
                    sendKeyState(VK_MEDIA_NEXT_TRACK, down, simulatedKeys);
                }
            }
        }
        if(keysPressed['F']){ // Media play/pause
            if(down){
                if(!simulatedKeys[VK_MEDIA_PLAY_PAUSE]){
                    sendKeyState(VK_MEDIA_PLAY_PAUSE, down, simulatedKeys);
                }
            } else {
                if(keyIsShift || key == 'F'){
                    sendKeyState(VK_MEDIA_PLAY_PAUSE, down, simulatedKeys);
                }
            }
        }
        if(keysPressed['Q']){ // Volume mute
            if(down){
                if(!simulatedKeys[VK_VOLUME_MUTE]){
                    sendKeyState(VK_VOLUME_MUTE, down, simulatedKeys);
                }
            } else {
                if(keyIsShift || key == 'Q'){
                    sendKeyState(VK_VOLUME_MUTE, down, simulatedKeys);
                }
            }
        }
    }

    if(!down) keysPressed[key] = false;
    std::cout << "checkCombo() end\n";
}


void unpressSimulatedKeys(std::unordered_map<DWORD, bool> &simulatedKeys){
    std::cout << "unpressSimulatedKeys() called\n";
    for(std::pair<DWORD, bool> key : simulatedKeys){
        sendKeyState(key.first, false, simulatedKeys);
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
