#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <vector>

const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 1000;
const int GRID_ROWS = 8;
const int GRID_COLS = 8;
const int TILE_WIDTH = 100;  // Twice the height for true isometric
const int TILE_HEIGHT = 50;

// Add at the top or near your globals:
int selectedI = -1, selectedJ = -1;
std::vector<std::pair<int, int>> validMoves;

// Helper to clamp values
int clamp(int v, int min, int max) {
    return v < min ? min : (v > max ? max : v);
}


enum PieceType { NONE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING };
enum PieceColor { EMPTY, WHITE, BLACK };

struct Piece {
    PieceType type;
    PieceColor color;
};

const char* pieceChar(PieceType type) {
    switch (type) {
    case PAWN:   return "P";
    case KNIGHT: return "N";
    case BISHOP: return "B";
    case ROOK:   return "R";
    case QUEEN:  return "Q";
    case KING:   return "K";
    default:     return "";
    }
}

// Initial chessboard setup
Piece board[GRID_ROWS][GRID_COLS] = {
    { {ROOK,WHITE}, {KNIGHT,WHITE}, {BISHOP,WHITE}, {QUEEN,WHITE}, {KING,WHITE}, {BISHOP,WHITE}, {KNIGHT,WHITE}, {ROOK,WHITE} },
    { {PAWN,WHITE}, {PAWN,WHITE},   {PAWN,WHITE},   {PAWN,WHITE},  {PAWN,WHITE}, {PAWN,WHITE},   {PAWN,WHITE},   {PAWN,WHITE} },
    { {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY},  {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY} },
    { {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY},  {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY} },
    { {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY},  {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY} },
    { {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY},  {NONE,EMPTY}, {NONE,EMPTY},   {NONE,EMPTY},   {NONE,EMPTY} },
    { {PAWN,BLACK}, {PAWN,BLACK},   {PAWN,BLACK},   {PAWN,BLACK},  {PAWN,BLACK}, {PAWN,BLACK},   {PAWN,BLACK},   {PAWN,BLACK} },
    { {ROOK,BLACK}, {KNIGHT,BLACK}, {BISHOP,BLACK}, {QUEEN,BLACK}, {KING,BLACK}, {BISHOP,BLACK}, {KNIGHT,BLACK}, {ROOK,BLACK} }
};

// Draw a filled circle (simple midpoint algorithm)
void drawCircle(SDL_Renderer* renderer, int cx, int cy, int radius)
{
    for (int w = 0; w < radius * 2; w++)
    {
        for (int h = 0; h < radius * 2; h++)
        {
            int dx = radius - w;
            int dy = radius - h;
            if ((dx * dx + dy * dy) <= (radius * radius))
            {
                SDL_RenderPoint(renderer, cx + dx, cy + dy);
            }
        }
    }
}

void drawPawn(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    drawCircle(renderer, cx, cy, TILE_HEIGHT / 4);
}

void drawRook(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    int w = TILE_HEIGHT / 2;
    int h = TILE_HEIGHT / 2;
    for (int y = -h / 2; y <= h / 2; ++y)
        SDL_RenderLine(renderer, cx - w / 2, cy + y, cx + w / 2, cy + y);
    // Draw battlements
    SDL_RenderLine(renderer, cx - w / 2, cy - h / 2, cx - w / 4, cy - h / 2 - 6);
    SDL_RenderLine(renderer, cx - w / 4, cy - h / 2 - 6, cx, cy - h / 2);
    SDL_RenderLine(renderer, cx, cy - h / 2, cx + w / 4, cy - h / 2 - 6);
    SDL_RenderLine(renderer, cx + w / 4, cy - h / 2 - 6, cx + w / 2, cy - h / 2);
}

void drawKnight(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    SDL_FPoint points[4] = {
        { (float)(cx - TILE_HEIGHT / 4), (float)(cy + TILE_HEIGHT / 4) },
        { (float)(cx), (float)(cy - TILE_HEIGHT / 4) },
        { (float)(cx + TILE_HEIGHT / 4), (float)(cy) },
        { (float)(cx - TILE_HEIGHT / 4), (float)(cy + TILE_HEIGHT / 4) }
    };
    SDL_RenderLines(renderer, points, 4);
    // Fill triangle (simple scanline)
    for (int y = 0; y < TILE_HEIGHT / 4; ++y)
        SDL_RenderLine(renderer, cx - y / 2, cy + y, cx + y / 2, cy + y);
}

void drawBishop(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    // Draw ellipse (vertical)
    int a = TILE_HEIGHT / 6;
    int b = TILE_HEIGHT / 3;
    for (int y = -b; y <= b; ++y) {
        int x = (int)(a * sqrt(1.0 - (y * y) / (double)(b * b)));
        SDL_RenderLine(renderer, cx - x, cy + y, cx + x, cy + y);
    }
    // Draw slash
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderLine(renderer, cx - a / 2, cy - b / 2, cx + a / 2, cy + b / 2);
}

void drawQueen(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    drawCircle(renderer, cx, cy, TILE_HEIGHT / 3);
    // Draw crown
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderLine(renderer, cx, cy - TILE_HEIGHT / 3, cx, cy - TILE_HEIGHT / 2);
    SDL_RenderLine(renderer, cx, cy - TILE_HEIGHT / 2, cx - 6, cy - TILE_HEIGHT / 2 + 8);
    SDL_RenderLine(renderer, cx, cy - TILE_HEIGHT / 2, cx + 6, cy - TILE_HEIGHT / 2 + 8);
}

void drawKing(SDL_Renderer* renderer, int cx, int cy, int color) {
    SDL_SetRenderDrawColor(renderer, color, color, color, 255);
    drawCircle(renderer, cx, cy, TILE_HEIGHT / 3);
    // Draw cross
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderLine(renderer, cx, cy - TILE_HEIGHT / 2, cx, cy - TILE_HEIGHT / 3);
    SDL_RenderLine(renderer, cx - 6, cy - TILE_HEIGHT / 2 + 8, cx + 6, cy - TILE_HEIGHT / 2 + 8);
}

void drawPiece(SDL_Renderer* renderer, int centerX, int centerY, const Piece& piece)
{
    if (piece.type == NONE) return;

    int color = (piece.color == WHITE) ? 220 : 40;

    switch (piece.type) {
    case PAWN:   drawPawn(renderer, centerX, centerY, color); break;
    case ROOK:   drawRook(renderer, centerX, centerY, color); break;
    case KNIGHT: drawKnight(renderer, centerX, centerY, color); break;
    case BISHOP: drawBishop(renderer, centerX, centerY, color); break;
    case QUEEN:  drawQueen(renderer, centerX, centerY, color); break;
    case KING:   drawKing(renderer, centerX, centerY, color); break;
    default: break;
    }
}
// Helper to check if a position is on the board
bool onBoard(int i, int j) {
    return i >= 0 && i < GRID_ROWS && j >= 0 && j < GRID_COLS;
}

// Add valid moves for each piece type
void getValidMoves(int i, int j, std::vector<std::pair<int, int>>& moves) {
    moves.clear();
    const Piece& p = board[i][j];
    if (p.type == NONE) return;

    int dir = (p.color == WHITE) ? 1 : -1;

    switch (p.type) {
    case PAWN:
        // Forward move
        if (onBoard(i + dir, j) && board[i + dir][j].type == NONE)
            moves.emplace_back(i + dir, j);
        // First move double
        if ((p.color == WHITE && i == 1) || (p.color == BLACK && i == 6)) {
            if (onBoard(i + dir, j) && board[i + dir][j].type == NONE &&
                onBoard(i + 2 * dir, j) && board[i + 2 * dir][j].type == NONE)
                moves.emplace_back(i + 2 * dir, j);
        }
        // Captures
        for (int dj = -1; dj <= 1; dj += 2) {
            int ni = i + dir, nj = j + dj;
            if (onBoard(ni, nj) && board[ni][nj].type != NONE && board[ni][nj].color != p.color)
                moves.emplace_back(ni, nj);
        }
        break;
    case KNIGHT:
        for (int di = -2; di <= 2; ++di) for (int dj = -2; dj <= 2; ++dj) {
            if (std::abs(di * dj) == 2) {
                int ni = i + di, nj = j + dj;
                if (onBoard(ni, nj) && (board[ni][nj].type == NONE || board[ni][nj].color != p.color))
                    moves.emplace_back(ni, nj);
            }
        }
        break;
    case BISHOP:
        for (int d = 1; d < GRID_ROWS; ++d) {
            for (int dx : {-1, 1}) for (int dy : {-1, 1}) {
                int ni = i + d * dx, nj = j + d * dy;
                if (!onBoard(ni, nj)) continue;
                if (board[ni][nj].type == NONE)
                    moves.emplace_back(ni, nj);
                else {
                    if (board[ni][nj].color != p.color) moves.emplace_back(ni, nj);
                    break;
                }
            }
        }
        break;
    case ROOK:
        for (int d = 1; d < GRID_ROWS; ++d) {
            for (int dx : {-1, 1}) {
                int ni = i + d * dx, nj = j;
                if (!onBoard(ni, nj)) continue;
                if (board[ni][nj].type == NONE)
                    moves.emplace_back(ni, nj);
                else {
                    if (board[ni][nj].color != p.color) moves.emplace_back(ni, nj);
                    break;
                }
            }
            for (int dy : {-1, 1}) {
                int ni = i, nj = j + d * dy;
                if (!onBoard(ni, nj)) continue;
                if (board[ni][nj].type == NONE)
                    moves.emplace_back(ni, nj);
                else {
                    if (board[ni][nj].color != p.color) moves.emplace_back(ni, nj);
                    break;
                }
            }
        }
        break;
    case QUEEN:
        // Combine rook and bishop
        getValidMoves(i, j, moves); // This would recurse infinitely, so instead:
        {
            std::vector<std::pair<int, int>> bMoves, rMoves;
            // Bishop moves
            for (int d = 1; d < GRID_ROWS; ++d) {
                for (int dx : {-1, 1}) for (int dy : {-1, 1}) {
                    int ni = i + d * dx, nj = j + d * dy;
                    if (!onBoard(ni, nj)) continue;
                    if (board[ni][nj].type == NONE)
                        bMoves.emplace_back(ni, nj);
                    else {
                        if (board[ni][nj].color != p.color) bMoves.emplace_back(ni, nj);
                        break;
                    }
                }
            }
            // Rook moves
            for (int d = 1; d < GRID_ROWS; ++d) {
                for (int dx : {-1, 1}) {
                    int ni = i + d * dx, nj = j;
                    if (!onBoard(ni, nj)) continue;
                    if (board[ni][nj].type == NONE)
                        rMoves.emplace_back(ni, nj);
                    else {
                        if (board[ni][nj].color != p.color) rMoves.emplace_back(ni, nj);
                        break;
                    }
                }
                for (int dy : {-1, 1}) {
                    int ni = i, nj = j + d * dy;
                    if (!onBoard(ni, nj)) continue;
                    if (board[ni][nj].type == NONE)
                        rMoves.emplace_back(ni, nj);
                    else {
                        if (board[ni][nj].color != p.color) rMoves.emplace_back(ni, nj);
                        break;
                    }
                }
            }
            moves.insert(moves.end(), bMoves.begin(), bMoves.end());
            moves.insert(moves.end(), rMoves.begin(), rMoves.end());
        }
        break;
    case KING:
        for (int di = -1; di <= 1; ++di) for (int dj = -1; dj <= 1; ++dj) {
            if (di == 0 && dj == 0) continue;
            int ni = i + di, nj = j + dj;
            if (onBoard(ni, nj) && (board[ni][nj].type == NONE || board[ni][nj].color != p.color))
                moves.emplace_back(ni, nj);
        }
        break;
    default: break;
    }
}

void drawBoard(SDL_Renderer* renderer, int hoverI = -1, int hoverJ = -1)
{
    int boardPixelWidth = (GRID_ROWS + GRID_COLS) * (TILE_WIDTH / 2);
    int boardPixelHeight = (GRID_ROWS + GRID_COLS) * (TILE_HEIGHT / 2);

    int xOffset = (SCREEN_WIDTH - boardPixelWidth) / 2;
    int yOffset = (SCREEN_HEIGHT - boardPixelHeight) / 2;

    for (int i = 0; i < GRID_ROWS; i++)
    {
        for (int j = 0; j < GRID_COLS; j++)
        {
            int centerX = (i - j) * (TILE_WIDTH / 2) + (boardPixelWidth / 2) + xOffset;
            int centerY = (i + j) * (TILE_HEIGHT / 2) + (TILE_HEIGHT / 2) + yOffset;

            SDL_FPoint points[5];
            points[0] = { static_cast<float>(centerX), static_cast<float>(centerY - TILE_HEIGHT / 2) }; // top
            points[1] = { static_cast<float>(centerX + TILE_WIDTH / 2), static_cast<float>(centerY) }; // right
            points[2] = { static_cast<float>(centerX), static_cast<float>(centerY + TILE_HEIGHT / 2) }; // bottom
            points[3] = { static_cast<float>(centerX - TILE_WIDTH / 2), static_cast<float>(centerY) }; // left
            points[4] = points[0]; // close the diamond

            // Highlight if hovered
            bool isValidMove = false;
            for (const auto& mv : validMoves) {
                if (mv.first == i && mv.second == j) {
                    isValidMove = true;
                    break;
                }
            }
            if (isValidMove) {
                SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green for valid move
            }
            else if (i == hoverI && j == hoverJ) {
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF); // Yellow
            }
            else if ((i + j) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); // White
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0x88, 0x88, 0x88, 0xFF); // Gray
            }

            // Fill the diamond (draw horizontal lines between left and right points)
            for (int y = -TILE_HEIGHT / 2; y <= TILE_HEIGHT / 2; ++y)
            {
                float progress = (float)y / (TILE_HEIGHT / 2);
                int halfWidth = static_cast<int>((TILE_WIDTH / 2) * (1.0f - std::abs(progress)));
                int drawY = centerY + y;
                SDL_RenderLine(renderer, centerX - halfWidth, drawY, centerX + halfWidth, drawY);
            }

            // Draw the outline in black
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
            SDL_RenderLines(renderer, points, 5);

            // Draw the piece if present
            int pieceCenterY = centerY;
            if (i == hoverI && j == hoverJ && board[i][j].type != NONE) {
                pieceCenterY -= 10; // Move up by 10 pixels when hovered
            }
            drawPiece(renderer, centerX, pieceCenterY, board[i][j]);
        }
    }
}

// Convert screen (mouse) coordinates to board (i, j)
bool screenToBoard(int mouseX, int mouseY, int& outI, int& outJ)
{
    int boardPixelWidth = (GRID_ROWS + GRID_COLS) * (TILE_WIDTH / 2);
    int boardPixelHeight = (GRID_ROWS + GRID_COLS) * (TILE_HEIGHT / 2);

    int xOffset = (SCREEN_WIDTH - boardPixelWidth) / 2;
    int yOffset = (SCREEN_HEIGHT - boardPixelHeight) / 2;

    // Translate mouse coordinates to board space
    float mx = mouseX - (boardPixelWidth / 2) - xOffset;
    float my = mouseY - (TILE_HEIGHT / 2) - yOffset;

    // Inverse isometric transform
    float fi = (my / (TILE_HEIGHT / 2) + mx / (TILE_WIDTH / 2)) / 2.0f;
    float fj = (my / (TILE_HEIGHT / 2) - mx / (TILE_WIDTH / 2)) / 2.0f;

    int i = static_cast<int>(std::floor(fi + 0.5f));
    int j = static_cast<int>(std::floor(fj + 0.5f));

    if (i >= 0 && i < GRID_ROWS && j >= 0 && j < GRID_COLS)
    {
        // Check if mouse is inside the diamond
        int centerX = (i - j) * (TILE_WIDTH / 2) + (boardPixelWidth / 2) + xOffset;
        int centerY = (i + j) * (TILE_HEIGHT / 2) + (TILE_HEIGHT / 2) + yOffset;
        int dx = std::abs(mouseX - centerX);
        int dy = std::abs(mouseY - centerY);
        if (dx * TILE_HEIGHT / 2 + dy * TILE_WIDTH / 2 <= (TILE_WIDTH * TILE_HEIGHT) / 4) {
            outI = i;
            outJ = j;
            return true;
        }
    }
    return false;
}

int main(int argc, char* args[])
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    window = SDL_CreateWindow("10x20 Grid SDL3", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == nullptr)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;
    int hoverI = -1, hoverJ = -1;

    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }
            else if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                int mx = e.motion.x;
                int my = e.motion.y;
                int i, j;
                if (screenToBoard(mx, my, i, j)) {
                    hoverI = i;
                    hoverJ = j;
                }
                else {
                    hoverI = -1;
                    hoverJ = -1;
                }
            }
            else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                int mx = e.button.x;
                int my = e.button.y;
                int i, j;
                if (screenToBoard(mx, my, i, j) && board[i][j].type != NONE) {
                    selectedI = i;
                    selectedJ = j;
                    getValidMoves(i, j, validMoves);
                }
                else {
                    selectedI = selectedJ = -1;
                    validMoves.clear();
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF); // Black background
        SDL_RenderClear(renderer);

        drawBoard(renderer, hoverI, hoverJ);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}