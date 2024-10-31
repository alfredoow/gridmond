#include <iostream>
#include "raylib.h"
#include "raymath.h"

int window_width = 1280;
int window_height = 720;
const char* window_title = "Gridmond";
Color window_color = BLACK;

const int size = 256;
float target_scale = 4.0f;
float scale = target_scale;
int brush_size = 1;

int mx = 0;
int my = 0;
int mz = 0;

float px = -window_width / 2 + size / 2 * scale;
float py = -window_height / 2 + size / 1.5 * scale;

float camera_speed = 5.0f;

int ground_level = 32;

float zoom = 1.0f;

enum CellType {
	None,
	Solid,
	Dust,
	Liquid,
	Gas
};

struct Cell {
	uint8_t id;
	Color color;
	int density;
	CellType type = CellType::None;

	bool operator==(Cell o) const {
		return id == o.id;
	}

	Cell* ptr;
	Cell* operator->() {
		return ptr;
	};
};

Cell empty = { 0, {20, 20, 20, 255}, 0, CellType::None };
Cell sand = { 1, {255, 219, 157, 255}, 2, CellType::Dust };
Cell brick = { 2, {100, 100, 100, 255}, 99, CellType::Solid };
Cell water = { 3, {0, 80, 240, 255}, 1, CellType::Liquid };

Cell cells[size * size];
Cell selectedCell = sand;

uint8_t pixels[size * size * 4];
Image image = { pixels, size, size, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
Texture2D texture;

Camera2D camera;

bool inBounds(int x, int y);
bool isEmpty(int x, int y, int d);
void setCell(int x, int y, Cell c);
Cell getCell(int x, int y);
void swapCell(int x, int y, int x1, int y1);
void drawCellSquare(int x, int y, int s, Cell c);
void updateCells(float dt);

void setPixel(uint8_t pixels[], int x, int y, Color c);

int main() {
	InitWindow(window_width, window_height, window_title);
	SetWindowIcon(LoadImage("resources/gridmond.png"));
	SetWindowState(FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(60);

	for (int x = 0; x < size; x++) {
		for (int y = 0; y < size; y++) {
			setCell(x, y, empty);
		}
	}

	for (int x = 0; x < size; x++) {
		for (int y = size - ground_level; y < size; y++) {
			setCell(x, y, sand);
		}
	}

	texture = LoadTextureFromImage(image);

	camera = { 0 };
	camera.target = { px, py };
	camera.zoom = 1.0f;

	while (WindowShouldClose() != true) {
		float dt = GetFrameTime();

		updateCells(dt);

		if (IsKeyDown(KEY_G)) {
			std::cout << scale << std::endl;
		}

		camera.target = { px, py };

		int s = brush_size * scale;

		BeginDrawing();
		BeginMode2D(camera);
		ClearBackground(window_color);
		DrawTexturePro(texture, { 0, 0, size, size }, { 0, 0, size * scale, size * scale}, {0, 0}, 0.0f, {255, 255, 255, 255});
		DrawRectangle(mx - s / 2, my - s / 2, s, s, selectedCell.color);
		DrawRectangleLines(mx - s / 2, my - s / 2, s, s, GREEN);
		EndMode2D();
		EndDrawing();
	}

	UnloadTexture(texture);
	CloseWindow();
	return 0;
}

void updateCells(float dt) {
	mx = px + GetMouseX();
	my = py + GetMouseY();
	mz = Clamp(GetMouseWheelMove(), -1, 1);

	int x = mx / scale;
	int y = my / scale;

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		std::cout << "(" << mx << "," << my << ")" << " (" << x << "," << y << ")" << std::endl;
		drawCellSquare(x, y, brush_size, selectedCell);
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
		std::cout << "(" << mx << "," << my << ")" << " (" << x << "," << y << ")" << std::endl;
		drawCellSquare(x, y, brush_size, empty);
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
		px -= GetMouseDelta().x;
		py -= GetMouseDelta().y;
	}

	if (IsKeyDown(KEY_LEFT_SHIFT)) {
		brush_size = mz != 0 ? brush_size + mz : brush_size;
		brush_size = Clamp(brush_size, 1, 64);
	} else {
		target_scale = mz != 0 ? target_scale + mz * 0.25f : target_scale;
	}

	if (IsKeyPressed(KEY_ONE)) {
		selectedCell = sand;
	}

	if (IsKeyPressed(KEY_TWO)) {
		selectedCell = brick;
	}

	if (IsKeyPressed(KEY_THREE)) {
		selectedCell = water;
	}

	target_scale = Clamp(target_scale, 1, 100);
	scale = Lerp(scale, target_scale, 8.0f * dt);

	for (int x = 0; x < size; x++) {
		for (int y = size - 1; y >= 0; y--) {
			int r = GetRandomValue(-1, 1);
			CellType type = cells[x + y * size].type;

			switch (type) {
			case CellType::None || CellType::Solid:
				continue;
			break;
			case CellType::Dust :
				if (isEmpty(x, y + 1, getCell(x, y).density)) {
					swapCell(x, y, x, y + 1);
				} else if (isEmpty(x + r, y + 1, getCell(x, y).density) && isEmpty(x + r, y, getCell(x, y).density)) {
					swapCell(x, y, x + r, y + 1);
				}
			break;
			case CellType::Liquid:
				if (isEmpty(x, y + 1, getCell(x, y).density)) {
					swapCell(x, y, x, y + 1);
				} else if (isEmpty(x + r, y + 1, getCell(x, y).density) && isEmpty(x + r, y, getCell(x, y).density)) {
					swapCell(x, y, x + r, y + 1);
				} else if (isEmpty(x + r, y, getCell(x, y).density)) {
					swapCell(x, y, x + r, y);
				}
			break;
			}
		}
	}

	image.data = pixels;
	UpdateTexture(texture, image.data);
}

void setCell(int x, int y, Cell c) {
	if (!inBounds(x, y)) { return; }
	cells[x + y * size] = c;
	setPixel(pixels, x, y, c.color);
}

Cell getCell(int x, int y) {
	if (inBounds(x, y)) {
		return cells[x + y * size];
	}
}

void swapCell(int x, int y, int x1, int y1) {
	Cell tmp = getCell(x1, y1);
	setCell(x1, y1, getCell(x, y));
	setCell(x, y, tmp);
}

void drawCellSquare(int x, int y, int s, Cell c) {
	int half_s = s / 2;

	if (half_s >= 1) {
		for (int _x = x - half_s; _x < x + half_s; _x++) {
			for (int _y = y - half_s; _y < y + half_s; _y++) {
				setCell(round(_x), round(_y), c);
			}
		}
	} else {
		setCell(x, y, c);
	}
}

bool inBounds(int x, int y) {
	return x >= 0 && y >= 0 && x <= size - 1 && y <= size - 1;
}

bool isEmpty(int x, int y, int d = 99) {
	return inBounds(x, y) && (getCell(x, y) == empty || getCell(x, y).density < d);
}

bool isLessDense(Cell c, int x, int y) {
	return inBounds(x, y) && cells[x + y * size].density < c.density;
}

void setPixel(uint8_t pixels[], int x, int y, Color c) {
	int i = (x + y * size) * 4;

	if (c.r == sand.color.r && c.g == sand.color.g) {
		c = { (uint8_t)GetRandomValue(205, 255), (uint8_t)GetRandomValue(199, 219), (uint8_t)GetRandomValue(137, 157), 255 };
	}

	pixels[i + 0] = c.r;
	pixels[i + 1] = c.g;
	pixels[i + 2] = c.b;
	pixels[i + 3] = c.a;
}