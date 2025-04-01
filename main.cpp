#include <SDL.h>
#include <SDL_image.h>
#include <iostream>
#include <string>
#include <vector>
//hai thư viện dưới là để chọn từ random
#include <ctime>
#include <random>
#include <fstream>
#include <SDL_ttf.h>
#include <limits>
#include <cstdlib>
using namespace std;
enum GameState {
MENU,
EASY,
HARD,
WIN,
LOSE,
};
vector <string> words_from_file (const string &wordFilename)//hàm lấy dữ liệu từ file cất vào 1 vector với biến đầu vào là một file ;
                                                        //filename là tham chiếu đại diện cho file đó
{

    vector<string>words;
    ifstream file(wordFilename);//ifstream file() dùng để mở 1 file và cho phép sử dựng content của nó trong chương trình (bao gồm sao chép) nhưng k đc sửa đổi dữ liệu
                            //"file" ở đây về cơ bản là cách mình đặt tên file muốn đọc, có thể đặt ntn cũng được (có thể coi là tên đại diện cho file trong TT)
                            //ví dụ ifstream file1 ("Khiem.txt")
                            //      ifstream file2 ("Minh.txt")
                            //      getline (file2, word)  <--> đọc file của Minh

    string word;
    while (file >> word)  //đọc file đã được mở bằng ifstream và đưa copy vào word. Mỗi lần đọc thì đọc một hàng
    {
        words.push_back(word);
    }   //đưa từng hàng trong file vào vector words, mỗi phần tử của vector ứng với 1 "word"
    file.close();
    return words;
}
string choose_given_word (const vector<string> words_from_file){            //hàm chọn ngẫu nhiên 1 từ trong file (cụ thể hơn là mảng word_from_file{
if (words_from_file.empty()) {
    cout << "Error: Word list is empty!" << endl;
    return "";
}
    mt19937 rng(static_cast<unsigned int>(time(0)));

    uniform_int_distribution<int> dist(0, words_from_file.size() - 1);

    return words_from_file[dist(rng)];
}


bool init (SDL_Window*& window, SDL_Renderer*& renderer, int width, int height){       //hàm khởi tạo và chuẩn bị cho việc vẽ
    if(SDL_Init(SDL_INIT_VIDEO) <0){
        cout << "sdl init error\n";
        return false;
    }
    window = SDL_CreateWindow("my hangman game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,SDL_WINDOW_SHOWN);
    if(!window){
        cout <<"window init error\n";
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!renderer){
        cout <<"renderer init error\n";
        return false;
    }
    int imgFormat = IMG_INIT_PNG | IMG_INIT_JPG;
    if (!IMG_Init(imgFormat)&imgFormat){
        cout <<"img loading fail\n";
        return false;
    }
    if (TTF_Init() == -1) {
    cout << "TTF init error: " << TTF_GetError() << endl;
    return false;
    }

    return true;
}
SDL_Texture* Create_texture (SDL_Renderer* renderer,const string filename)            //hàm tạo texture chuẩn bị cho việc vẽ
{                                                                                  //hàm này sử dụng renderer sẵn có chứ không khởi tạo nó, nên không cần tham chiếu
    SDL_Surface* loaded_to_RAM = IMG_Load(filename.c_str());
    if(!loaded_to_RAM){
        cout <<"load to Surface failed\n";
        return nullptr;
    }
    SDL_Texture* img_texture = SDL_CreateTextureFromSurface(renderer, loaded_to_RAM);
    if(!img_texture){
        cout <<"texture error\n" ;
        return nullptr;
    }
    SDL_FreeSurface(loaded_to_RAM);
    return img_texture;
}
void drawing(SDL_Renderer* renderer ,SDL_Texture* texture)                           //hàm vẽ
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer,texture,NULL,NULL);
    SDL_RenderPresent(renderer);
}

struct Button {
    SDL_Rect rect;        // Vị trí và kích thước
    SDL_Texture* texture; // Texture của nút
    bool isHovered;       // Trạng thái hover
};
Button easy_mode, hard_mode, restart, quitgame;
 //sẽ khởi tạo bằng initButtons trong main
//vì bản thân các nút có ảnh hưởng đến gamemode chứ không phải texture đơn thuần(chỉ vẽ là được) nên phải xài hàm void thay vì SDL_Texture

void initButtons(Button& button ,SDL_Renderer* renderer, const string filename, int x, int y, int width, int height) {
    SDL_Surface* loaded_to_RAM_2 = IMG_Load(filename.c_str());
    if (!loaded_to_RAM_2) {
        cerr << "Failed to load buttons: " << IMG_GetError() << endl;
        return;
    }
    button.texture = SDL_CreateTextureFromSurface(renderer, loaded_to_RAM_2);
    button.rect = {x, y, width, height};
    SDL_FreeSurface(loaded_to_RAM_2);
}

void drawButton(SDL_Renderer* renderer, const Button& button) {         //chỉ đọc dữ liệu của button nên xài const đảm bảo nó không bị thay đổi
    SDL_RenderCopy(renderer, button.texture, NULL, &button.rect);
}
void drawBackground(SDL_Renderer* renderer, SDL_Texture* backgroundTexture){ //bản thân hàm Create_t đã tạo biến SDL_Texture có tên riêng và cũng đã xác định luôn file gốc của nó rồi
    SDL_RenderCopy(renderer,backgroundTexture, NULL,NULL );
}
void drawMainMenu(SDL_Renderer* renderer, SDL_Texture* GameStartBg) {
    SDL_RenderClear(renderer);
    drawBackground(renderer, GameStartBg);
    drawButton(renderer, easy_mode);
    drawButton(renderer, hard_mode);
    SDL_RenderPresent(renderer);
}
void drawGameOver(SDL_Renderer* renderer, SDL_Texture* GameOverBg) {  //GameOverBg có thể là texture của win hoặc lose
    SDL_RenderClear(renderer);
    drawBackground(renderer, GameOverBg);
    drawButton(renderer, restart);
    drawButton(renderer, quitgame);
    SDL_RenderPresent(renderer);
}

void runGame(SDL_Renderer* renderer, const vector<string>& secret_wordpool, GameState& currentState) {

    if (secret_wordpool.empty()) {
    cout << "Error: No words loaded. Check file path!" << endl;
    }
    string given_word = choose_given_word(secret_wordpool);
    string represented_word;
    for(int i=0; i<given_word.size(); i++)
    {
        represented_word+= '-';
    }
    cout << represented_word << "\n";
    string filename = "hangman0.png";  // Khởi tạo filename
    SDL_Texture* textures = Create_texture( renderer,filename);
    if (!textures) {
        cout << "Failed to load texture: " << filename << endl;
    }
    drawing(renderer, textures);

    int falsecount = 0;
    bool gameRunning = true;

while (gameRunning && represented_word != given_word && falsecount <6) {
    SDL_PumpEvents();  // Thay do-while bằng while
    SDL_Event event;

    while (SDL_PollEvent(&event)) {  // Liên tục kiểm tra sự kiện
        if (event.type == SDL_QUIT) {
            currentState = LOSE;
            gameRunning = false;
        }


        if (event.type == SDL_KEYDOWN){

        char guessword = tolower( static_cast<char>(event.key.keysym.sym));
        //cout <<"guessword is: \n";
        cout << "guessed word is: \n";
        cout<< guessword <<endl;

        int sameletter = 0;
        for (int i = 0; i < given_word.size(); i++) {
            if (guessword == given_word[i]) {
                represented_word[i] = given_word[i];
                sameletter++;
            }
        }
        if (sameletter > 0) {
            cout << "you are right\n";
            cout << represented_word << "\n";
            cout <<endl;
        }
        else {
            cout << "you are wrong\n";
            cout <<endl;
            falsecount++;
            switch (falsecount) {
                case 1: filename = "hangman1.png"; break;
                case 2: filename = "hangman2.png"; break;
                case 3: filename = "hangman3.png"; break;
                case 4: filename = "hangman4.png"; break;
                case 5: filename = "hangman5.png"; break;
                case 6: filename = "hangman6.png"; break;
                default: filename = "hangman0.png"; break;
            }

            SDL_DestroyTexture(textures);
            textures = Create_texture(renderer, filename);   //cập nhật lại texture khi falsecount thay đổi
            if (!textures) {
            cout << "Failed to load texture: " << filename << endl;
            }
        }
        }

    }
        drawing(renderer, textures);
}

        if (represented_word == given_word) {
        cout << "you win! congratulation\n";
        currentState = WIN;
        }

        else if (falsecount == 6) {
        cout <<"answer is :" << given_word <<endl;
        cout << "you lose\n";
        currentState = LOSE;
        }
        SDL_DestroyTexture(textures);

}

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    if (!init(window, renderer, 1000, 500)) {
        return -1;
    }
    //Vẽ màn hình khởi đầu/kết thúc
    SDL_Texture* menuBg = Create_texture(renderer, "gamestart_background.jpg");
    SDL_Texture* winBg = Create_texture(renderer, "gameover_win_background.jpg");
    SDL_Texture* loseBg = Create_texture(renderer, "gameover_lose_background.png");
    if (!menuBg || !winBg || !loseBg) {
    cout << "Failed to load menu background!" << endl;
    return -1;
    }
    initButtons(easy_mode, renderer, "easy_button.png", 125, 312, 250, 125);
    initButtons(hard_mode, renderer, "hard_button.png", 625, 312, 250, 125);
    initButtons(restart, renderer, "restart_button.png", 125, 312, 250, 125);
    initButtons(quitgame, renderer, "quit_button.png", 625, 312, 250, 125);

    GameState currentState = MENU;
bool quit = false;
while(!quit)
{

    SDL_Event event;
    while (SDL_PollEvent(&event)) {       //vòng lặp xử lí sự kiện (ấn nút, thắng,thua, thoát game, etc)
        if (event.type == SDL_QUIT) {
            quit = true;
            break;
        }
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            SDL_Point mousePoint = {mouseX, mouseY};
            if(currentState ==MENU){
                if(SDL_PointInRect(&mousePoint, &easy_mode.rect)){
                    currentState = EASY;
                }
                if(SDL_PointInRect(&mousePoint, &hard_mode.rect)){
                    currentState = HARD;
                }
            }
            if(currentState == WIN ||currentState == LOSE){
                 if(SDL_PointInRect(&mousePoint, &restart.rect)){
                    currentState = MENU;
                 }
                 if(SDL_PointInRect(&mousePoint, &quitgame.rect)){
                    quit = true;
                    break;
                 }
            }
        }
    }
    vector<string> secret_wordpool;
    switch (currentState)
    {
        case MENU: drawMainMenu(renderer, menuBg); break;
        case EASY : {
                secret_wordpool = words_from_file("easy_wordpool.txt");
                runGame(renderer, secret_wordpool, currentState);
                break;
        }
        case HARD : {
                secret_wordpool = words_from_file("hard_wordpool.txt");
                runGame(renderer, secret_wordpool, currentState);
                break;
        }
        case WIN: drawGameOver(renderer, winBg); break;
        case LOSE: drawGameOver(renderer, loseBg); break;
    }
}
    // Giải phóng tài nguyên

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(easy_mode.texture);
    SDL_DestroyTexture(hard_mode.texture);
    SDL_DestroyTexture(restart.texture);
    SDL_DestroyTexture(quitgame.texture);
    SDL_DestroyTexture(menuBg);
    SDL_DestroyTexture(winBg);
    SDL_DestroyTexture(loseBg);
    IMG_Quit();
    SDL_Quit();
    return 0;
}


