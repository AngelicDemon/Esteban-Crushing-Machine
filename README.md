# Esteban Crushing Machine

## What is This?

This is a **REALLY UNFINISHED** hotkey program for **Windows** that uses Caps Lock as the main key. It aims to be a keyboard layer for the COLEMAK layout.
So far this project uses the Windows Low Level Keyboard Hook `WH_KEYBOARD_LL` to intercept Caps Lock and any keys pressed while Caps Lock is down, checks keys that have been pressed for any desired key combinations, and sends the desired input for the hotkey, if any.

## What is That Name??

The name is a play on words of the name Esteban and the meme Orphan-Crushing Machine.
Why, you may ask? This code is dedicated to my friend Esteban! The Orphan-Crushing Machine bit randomly came to mind and I was feeling a little silly at the time and decided to mash the two names together.

## Plans?

Absolutely everything is subject to change as nothing is really finalized.
So far, the list of things that work are
- Keyboard hook and hook procedure with `WH_KEYBOARD_LL`
- Combo checking works but is still actively being worked on
- Sending both up and down key events via `SendInput`

Plans that I have in mind but may or may not get to are
- Actually fixing up and completely implementing the intended hotkey combos
- Making the program run in the background without a console window
- Making a tray icon to exit the program
- Possibly implementing a class that handles making combinations so manually making a combination isn't needed
- Possibly implementing a Shell Hook to reregister hooks when troublesome programs (GTA V) get into focus