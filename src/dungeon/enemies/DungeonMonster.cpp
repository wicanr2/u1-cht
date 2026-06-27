#include "DungeonMonster.h"
#include "SDL2_gfxPrimitives.h"

// 在地牢第一人稱視野(304×144 viewport,中心 x=152、地板 y=131)繪製可縮放剪影。
// 黑底填滿 + 怪物色描邊 = U1 線框風;距離越遠越小。
void DungeonMonster::draw(SDL_Renderer *renderer, int distance) {
    if (distance < 1) distance = 1;
    if (distance > 4) distance = 4;
    static const float SCALE[5] = {0, 1.0f, 0.62f, 0.40f, 0.26f};
    float s = SCALE[distance];

    const int cx = 152, floorY = 131;
    const Uint8 R = _r, G = _g, B = _b, A = 255;
    const Uint8 KR = 0, KG = 0, KB = 0;   // 黑底

    auto boxF = [&](int x0, int y0, int x1, int y1) {
        boxRGBA(renderer, x0, y0, x1, y1, KR, KG, KB, A);
        rectangleRGBA(renderer, x0, y0, x1, y1, R, G, B, A);
    };
    auto circF = [&](int x, int y, int rad) {
        if (rad < 1) rad = 1;
        filledCircleRGBA(renderer, x, y, rad, KR, KG, KB, A);
        circleRGBA(renderer, x, y, rad, R, G, B, A);
    };
    auto lineC = [&](int x0, int y0, int x1, int y1) { lineRGBA(renderer, x0, y0, x1, y1, R, G, B, A); };
    auto triF = [&](int x0, int y0, int x1, int y1, int x2, int y2) {
        filledTrigonRGBA(renderer, x0, y0, x1, y1, x2, y2, KR, KG, KB, A);
        trigonRGBA(renderer, x0, y0, x1, y1, x2, y2, R, G, B, A);
    };

    switch (_shape) {
        case Shape::Humanoid: {
            int h = (int)(100 * s), w = (int)(34 * s), headR = (int)(13 * s);
            int top = floorY - h;
            circF(cx, top + headR, headR);                                  // 頭
            boxF(cx - w / 2, top + 2 * headR, cx + w / 2, floorY);          // 軀幹
            int sh = top + 2 * headR + (int)(8 * s);                        // 肩
            lineC(cx - w / 2, sh, cx - w, sh + (int)(22 * s));             // 左臂
            lineC(cx + w / 2, sh, cx + w, sh + (int)(22 * s));             // 右臂
            break;
        }
        case Shape::Beast: {
            int h = (int)(46 * s), w = (int)(74 * s);
            int top = floorY - h;
            boxF(cx - w / 2, top, cx + w / 2 - (int)(16 * s), floorY);      // 低身軀
            circF(cx + w / 2 - (int)(10 * s), top + (int)(6 * s), (int)(12 * s)); // 頭(前端)
            for (int i = 0; i < 4; i++) {                                   // 四腿
                int lx = cx - w / 2 + (int)((10 + i * 16) * s);
                lineC(lx, floorY, lx, floorY + (int)(10 * s));
            }
            break;
        }
        case Shape::Blob: {
            int rad = (int)(42 * s);
            circF(cx, floorY - rad, rad);                                   // 膠團
            circF(cx, floorY - rad, (int)(rad * 0.55f));                    // 內核
            break;
        }
        case Shape::Flyer: {
            int bodyR = (int)(16 * s);
            int cyc = floorY - (int)(60 * s);
            circF(cx, cyc, bodyR);                                          // 軀幹
            int wing = (int)(46 * s);
            triF(cx - bodyR, cyc, cx - bodyR - wing, cyc - (int)(20 * s), cx - bodyR - wing, cyc + (int)(20 * s)); // 左翼
            triF(cx + bodyR, cyc, cx + bodyR + wing, cyc - (int)(20 * s), cx + bodyR + wing, cyc + (int)(20 * s)); // 右翼
            lineC(cx, cyc + bodyR, cx, floorY);                            // 尾/腿
            break;
        }
    }
}
