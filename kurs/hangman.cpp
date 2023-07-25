#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <regex>
#include <cstdlib>
#include <Windows.h>
#include <mmsystem.h>
#include <winuser.h>
#include <tchar.h>
#include "Resource.h"

using namespace std;

HWND hwnd;
HWND lblHiddenWord;
HWND lblHint;
HWND lblScore;
HWND lblGuessedChars;
HWND audioButton;
int score = 0;
int currentLevel = 1;
int maxAttempts = 6;
int attemptsRemaining = 6;
int characterSpacing = 5;
bool gameStarted = false;
bool soundOn = false;
string hiddenWord;
string hint;
vector<string> hiddenWords;
vector<string> hints;
vector<string> guessedWords;
vector<string> displayedWords;
vector<string> displayedHints;
vector<char> guessedChars;

void DisplayRandomWord();
void ResetGame();
void UpdateHiddenWordLabel();
void UpdateGuessedCharsLabel();
void CheckGuess(char guess);
void UpdateScore(int addPoints);
void LoadWordList(const char* fileName);
void LevelUp(const char* level);
void SetButtonIcon(HINSTANCE hInstance, int icon);
void DrawHangman(HDC hdc, int attemptsRemaining);
pair<string, string> ChooseRandomWord();
pair<string, string> ChooseAndRemovePair(vector<string>& wordsList, vector<string>& hintsList, int randomIndex);
string ConvertStringToUpper(string str);
string GetWindowStringText(HWND hwnd);
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Create letter buttons for alternative input
void CreateAlphabetButtons(HWND hwnd, HINSTANCE hInstance)
{
    const int buttonWidth = 40;
    const int buttonHeight = 25;
    const int buttonMarginX = 10;
    const int buttonMarginY = 10;
    const int numRows = 9;
    const int numColumns = 3;

    int startX = 50;
    int startY = 200;

    HMENU menuItem = (HMENU)1000; // Starting HMENU ID for the buttons

    for (int row = 0; row < numRows; ++row) 
    {
        for (int col = 0; col < numColumns; ++col) 
        {
            char letter = 'A' + (row * numColumns + col);

            // Stop when it's not a letter
            if (!isalpha(letter))
                break;

            HWND button = CreateWindow(_T("BUTTON"), (LPCSTR)std::wstring(1, letter).c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, startX + col * (buttonWidth + buttonMarginX), startY + row * (buttonHeight + buttonMarginY), buttonWidth, buttonHeight, hwnd, menuItem, hInstance, NULL);

            // Increment the HMENU ID for the next button
            menuItem = (HMENU)(reinterpret_cast<INT_PTR>(menuItem) + 1);
        }
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Create the window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("HangmanGame"), NULL };
    RegisterClassEx(&wc);
    hwnd = CreateWindow(wc.lpszClassName, _T("Hangman Game"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);

    // Set app icon 
    HICON hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_KURS));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    // Label for displaying the hidden word
    lblHiddenWord = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE | SS_OWNERDRAW, 250, 50, 350, 40, hwnd, NULL, wc.hInstance, NULL);

    // Label for displaying the hint
    lblHint = CreateWindow(_T("STATIC"), _T(""), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_EDITCONTROL, 250, 100, 350, 40, hwnd, NULL, wc.hInstance, NULL);

    // Label for displaying guessed characters
    lblGuessedChars = CreateWindow(_T("STATIC"), _T("GUESSED LETTERS:"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_EDITCONTROL, 650, 50, 100, 90, hwnd, NULL, wc.hInstance, NULL);

    // Label for displaying the score
    lblScore = CreateWindow(_T("STATIC"), _T("SCORE: 0"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE, 50, 50, 140, 40, hwnd, NULL, wc.hInstance, NULL);

    // Button for sound on/off
    audioButton = CreateWindow(_T("BUTTON"), _T(""), WS_CHILD | WS_VISIBLE | BS_ICON, 50, 100, 40, 40, hwnd, (HMENU)119, wc.hInstance, NULL);
    SetButtonIcon(wc.hInstance, IDI_OFF);

    // Buttons with letters for alternative input
    CreateAlphabetButtons(hwnd, wc.hInstance);

    LoadWordList("easy.txt");
    DisplayRandomWord();
    UpdateHiddenWordLabel();

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    gameStarted = true;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear the background 
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255)); // White color
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CHAR:
    {
        // Check to prevent WM_CHAR from firing on app start (window focus)
        if (!gameStarted)
            break;

        char ch = static_cast<char>(wParam);
        if (isalpha(ch))
        {
            CheckGuess(toupper(ch));
        }
        else
        {
            MessageBox(NULL, "Sorry, that's not a valid input. Try english letters instead.", "Error", MB_OK);
        }
        break;
    }
    case WM_COMMAND:
    {
        int id = LOWORD(wParam);
        int event = HIWORD(wParam);

        // Not really necessary (as the app has only buttons), 
        // but it's good prectice and adds an extra level of validation
        if (event == BN_CLICKED)
        {
            // Check if sound on/off button was pressed
            if (id == 119)
            {
                HINSTANCE hInstance = GetModuleHandle(NULL);
                if (soundOn)
                {
                    // Stop sound and change the icon
                    sndPlaySound(NULL, SND_ASYNC);
                    SetButtonIcon(hInstance, IDI_OFF);
                }
                else
                {
                    // Play sound and change the icon
                    bool sound = sndPlaySound("music.wav", SND_LOOP | SND_ASYNC);
                    if (sound)
                    {
                        SetButtonIcon(hInstance, IDI_ON);
                    }
                    else
                    {
                        MessageBox(NULL, "Failed to play audio", "Error", MB_OK);
                    }
                }
                soundOn = !soundOn;
            }
            // Check if letter button was pressed
            else if (id >= 1000 && id <= 1025)
            {
                // Get the letter from the button's caption
                TCHAR buttonText[2];
                GetWindowText((HWND)lParam, buttonText, 2);
                char guess = static_cast<char>(buttonText[0]);

                // Pass the letter to the CheckGuess method
                CheckGuess(guess);
            }

            // Set focus back to main window to continue playing
            SetFocus(hwnd);
        }
        break;
    }
    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

        if (pdis->hwndItem == lblHiddenWord)
        {
            // Set the character spacing for the static control
            HDC hdc = pdis->hDC;
            SetTextCharacterExtra(hdc, characterSpacing);

            // Get the text of the label
            TCHAR text[256];
            GetWindowText(lblHiddenWord, text, 256);

            // Calculate the width and height for the label based on the text and character spacing
            SIZE textSize;
            GetTextExtentPoint32(hdc, text, lstrlen(text), &textSize);
            int labelWidth = textSize.cx + (characterSpacing * (lstrlen(text) - 1)) - 35;
            int labelHeight = textSize.cy;

            // Calculate the position to center the label both horizontally and vertically
            RECT rcItem = pdis->rcItem;
            int centerX = (rcItem.left + rcItem.right - labelWidth) / 2;
            int centerY = (rcItem.top + rcItem.bottom - labelHeight) / 2;

            // Draw the background rectangle with the default light gray color
            HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(hdc, &rcItem, hBrush);
            DeleteObject(hBrush);

            // Draw the text
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkMode(hdc, TRANSPARENT);
            ExtTextOut(hdc, centerX, centerY, ETO_CLIPPED, &rcItem, text, lstrlen(text), NULL);
        }

        break;
    }
    case WM_CLOSE:
    {
        // Show confirmation dialog before closing
        int result = MessageBox(NULL, "Are you sure you want to quit?", "Confirmation", MB_YESNO | MB_ICONQUESTION);
        if (result == IDYES)
        {
            PostQuitMessage(0);
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// Sets the icon for sound on/off button
void SetButtonIcon(HINSTANCE hInstance, int icon)
{
    HICON buttonIcon = LoadIcon(hInstance, MAKEINTRESOURCE(icon));
    SendMessage(audioButton, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)buttonIcon);
}

// Load word list from file
void LoadWordList(const char* fileName)
{
    ifstream file(fileName);
    if (!file.is_open())
    {
        MessageBox(NULL, "Failed to open file", "Error", MB_OK);
        exit(EXIT_FAILURE);
    }

    // Clear vectors from previous data
    hiddenWords.clear();
    hints.clear();
    guessedWords.clear();
    displayedWords.clear();
    displayedHints.clear();

    string line;
    while (getline(file, line))
    {
        // Each line in the file should contain a word followed by a hint, separated by a comma
        size_t commaPos = line.find(",");
        if (commaPos != string::npos)
        {
            string word = regex_replace(line.substr(0, commaPos), regex("^ +| +$|( ) +"), "$1");
            string hint = regex_replace(line.substr(commaPos + 1), regex("^ +| +$|( ) +"), "$1");
            if (!word.empty() && !hint.empty())
            {
                hiddenWords.push_back(ConvertStringToUpper(word));
                hints.push_back(ConvertStringToUpper(hint));
            }
        }
    }

    file.close();
}

// Convert string to upper case
string ConvertStringToUpper(string str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

// Choose a random word and hint from the loaded word list
void DisplayRandomWord()
{
    pair<string, string> wordHintPair = ChooseRandomWord();
    hiddenWord = wordHintPair.first;
    hint = wordHintPair.second;

    // End of the game (all of the words have been guessed)
    if (wordHintPair.first == "" && wordHintPair.second == "")
    {
        MessageBox(NULL, "Congratulations, you won the game! Starting over.", "Congratulations", MB_OK);

        score = 0;
        SetWindowText(lblScore, "SCORE: 0");
        currentLevel = 1;
        LoadWordList("easy.txt");
        ResetGame();
        return;
    }

    guessedChars.clear();
    attemptsRemaining = 6;

    // Update labels
    SetWindowText(lblHiddenWord, hiddenWord.c_str());
    SetWindowText(lblHint, hint.c_str());
}

// Choose random word. Unguessed words are displayed first. If there are no unseen words, 
// the word is chosen from those ones which were incorrectly guessed previously.
pair<string, string> ChooseRandomWord()
{
    vector<int> availableIndices;

    // Check if there are any words left in the initial vectors
    if (hiddenWords.empty())
    {
        // If all words have been displayed and guessed, return an empty pair
        if (displayedWords.empty() || all_of(displayedWords.begin(), displayedWords.end(),
            [&](const string& word) { return find(guessedWords.begin(), guessedWords.end(), word) != guessedWords.end(); }))
        {
            return make_pair("", "");
        }

        // Add indices of words that have been displayed but not guessed
        for (int i = 0; i < displayedWords.size(); ++i)
        {
            if (find(guessedWords.begin(), guessedWords.end(), displayedWords[i]) == guessedWords.end())
            {
                availableIndices.push_back(i);
            }
        }
    }
    else
    {
        // Add indices of words that have not been correctly guessed or displayed
        for (int i = 0; i < hiddenWords.size(); ++i)
        {
            if (find(guessedWords.begin(), guessedWords.end(), hiddenWords[i]) == guessedWords.end() &&
                find(displayedWords.begin(), displayedWords.end(), hiddenWords[i]) == displayedWords.end())
            {
                availableIndices.push_back(i);
            }
        }
    }

    // If there are no remaining words, return an empty pair
    if (availableIndices.empty())
    {
        return make_pair("", "");
    }

    // Choose a random index from the available indices
    srand(static_cast<unsigned int>(time(nullptr)));
    int randomIndex = availableIndices[rand() % availableIndices.size()];

    // Get the corresponding word and hint
    string randomWord;
    string randomHint;

    if (hiddenWords.empty())
    {
        return ChooseAndRemovePair(displayedWords, displayedHints, randomIndex);
    }
    else
    {
        return ChooseAndRemovePair(hiddenWords, hints, randomIndex);
    }
}

// Choose pair and remove it from the corresponding vector
pair<string, string> ChooseAndRemovePair(vector<string>& wordsList, vector<string>& hintsList, int randomIndex)
{
    string randomWord = wordsList[randomIndex];
    string randomHint = hintsList[randomIndex];

    wordsList.erase(wordsList.begin() + randomIndex);
    hintsList.erase(hintsList.begin() + randomIndex);

    return make_pair(randomWord, randomHint);
}

// Update the hidden word label to display underscores for unguessed characters
void UpdateHiddenWordLabel()
{
    string displayWord = "";
    for (char c : hiddenWord)
    {
        if (find(guessedChars.begin(), guessedChars.end(), c) != guessedChars.end())
        {
            displayWord += c;
        }
        else
        {
            displayWord += "_";
        }
    }

    SetWindowText(lblHiddenWord, displayWord.c_str());
}

// Update the guessed letters label
void UpdateGuessedCharsLabel()
{
    string guessedCharsString = "GUESSED LETTERS:\n\n";
    for (int i = 0; i < guessedChars.size(); i++)
    {
        guessedCharsString += string (1, guessedChars[i]) + " ";
    }

    SetWindowText(lblGuessedChars, guessedCharsString.c_str());
}

// Draw the hangman figure
void DrawHangman(HDC hdc, int attemptsRemaining)
{
    // Hangman base
    if (attemptsRemaining <= 5)
    {
        MoveToEx(hdc, 250, 500, NULL);
        LineTo(hdc, 450, 500);
    }

    // Vertical pole
    if (attemptsRemaining <= 4)
    {
        MoveToEx(hdc, 350, 500, NULL);
        LineTo(hdc, 350, 200);
    }

    // Horizontal pole
    if (attemptsRemaining <= 3)
    {
        MoveToEx(hdc, 350, 200, NULL);
        LineTo(hdc, 550, 200);
    }

    // Rope
    if (attemptsRemaining <= 2)
    {
        MoveToEx(hdc, 550, 200, NULL);
        LineTo(hdc, 550, 300);
    }

    // Head
    if (attemptsRemaining <= 1)
    {
        Ellipse(hdc, 525, 300, 575, 350);
    }

    if (attemptsRemaining <= 0)
    {
        // Body
        MoveToEx(hdc, 550, 350, NULL);
        LineTo(hdc, 550, 450);

        // Left arm
        MoveToEx(hdc, 550, 375, NULL);
        LineTo(hdc, 500, 400);

        // Right arm
        MoveToEx(hdc, 550, 375, NULL);
        LineTo(hdc, 600, 400);

        // Left leg
        MoveToEx(hdc, 550, 450, NULL);
        LineTo(hdc, 500, 500);

        // Right leg
        MoveToEx(hdc, 550, 450, NULL);
        LineTo(hdc, 600, 500);
    }
}

// Check the guessed character and update game state
void CheckGuess(char guess)
{
    if (find(guessedChars.begin(), guessedChars.end(), guess) != guessedChars.end())
    {
        MessageBox(NULL, "You have already guessed this character.", "Info", MB_OK);
        return;
    }

    guessedChars.push_back(guess);
    UpdateGuessedCharsLabel();
    UpdateHiddenWordLabel();

    // Guessed character is not in the hidden word
    if (hiddenWord.find(guess) == string::npos)
    {
        attemptsRemaining--;
        HDC hdc = GetDC(hwnd);
        DrawHangman(hdc, attemptsRemaining);
        ReleaseDC(hwnd, hdc);

        if (attemptsRemaining == 0)
        {
            // Add the word to the displayed words list
            displayedWords.push_back(hiddenWord);
            displayedHints.push_back(hint);

            // Player lost, show game over message
            string str = "You lost! The correct word was: " + hiddenWord;
            MessageBox(NULL, str.c_str(), "Oh no!", MB_OK);
            UpdateScore(-1 * currentLevel);
            ResetGame();
        }
    }

    // Check if the word has been guessed correctly
    string s = GetWindowStringText(lblHiddenWord);
    const char* str1 = s.c_str();
    const char* str2 = hiddenWord.c_str();
    if (strcmp(str1, str2) == 0)
    {
        // Add the word to the guessed words list
        guessedWords.push_back(hiddenWord);

        UpdateScore(3 * currentLevel);
        ResetGame();
    }
}

// Retrieve text from passed window
string GetWindowStringText(HWND hwnd)
{
    int len = GetWindowTextLength(hwnd) + 1;
    std::string s;
    s.reserve(len);
    GetWindowText(hwnd, const_cast<char*>(s.c_str()), len);
    return s;
}

// Update score. Negative points are passed in case the user incorrectly guesses a word
void UpdateScore(int points)
{
    if (points > 0 || points < 0 && fabs(score) >= fabs(points))
    {
        score += points;
        std::string str = "SCORE: " + to_string(score);
        SetWindowText(lblScore, str.c_str());

        // Check if the current score is enough or there's no words left to increase the difficulty level
        if (currentLevel == 1 && (score >= 60 || (displayedWords.size() == 0 && hiddenWords.size() == 0)))
        {
            LevelUp("medium.txt");
        }
        else if (currentLevel == 2 && (score >= 160 || (displayedWords.size() == 0 && hiddenWords.size() == 0)))
        {
            LevelUp("hard.txt");
        }
    }
}

// Show level up message and load next level words
void LevelUp(const char* level)
{
    MessageBox(NULL, "Congratulations, you're jumping on the next level now!", "Congrats!", MB_OK);
    LoadWordList(level);
    currentLevel++;
}

// Reset game. Clear characters, reset attempts, choose new word, update labels and erase the hangman
void ResetGame()
{
    gameStarted = false;

    // Clear guessed characters and reset remaining attempts
    guessedChars.clear();
    attemptsRemaining = maxAttempts;

    // Get a new random word and hint
    DisplayRandomWord();

    // Update labels
    SetWindowText(lblHiddenWord, string(hiddenWord.length(), '_').c_str());
    SetWindowText(lblHint, hint.c_str());
    UpdateGuessedCharsLabel();

    // Erase the hangman figure
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);

    gameStarted = true;
}