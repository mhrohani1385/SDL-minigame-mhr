#include <iostream>
#include <SDL2/SDL_main.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <cctype>

#ifdef __unix__
#ifndef __GFX__
#include <SDL2/SDL2_gfxPrimitives.h>
#define __GFX__
#endif
#endif

#ifdef __linux__
#ifndef __GFX__
#include <SDL2/SDL2_gfxPrimitives.h>
#define __GFX__
#endif
#endif

#ifndef __GFX__
#include <SDL2/SDL_gfx>
#endif

#define DATA_FILE_ADDR "./data/games/"

#define CHAR_RAW_ROOT "./raw/Char/"
#define FONT_ADDR "./arial.ttf"

#define DELAY 40
#define GRAVITY 10

#define WIDTH 1200
#define HEIGHT 800
#define BOTTOM_MARGIN 100

#define CHAR_HEIGHT 150

using namespace std;

//-----===== OK! Tested ====-----
bool draw_image_on_point(SDL_Renderer *renderer, SDL_Point center_point, Uint16 height, const char *image_address, SDL_Rect *srcrect = NULL, SDL_Rect *dstrect = NULL)
{
    SDL_Surface *surf = IMG_Load(image_address);
    if (!surf)
    {
        return false;
    }

    float scale = (float)height / (float)surf->h;
    SDL_Rect rect = {center_point.x - (int)(surf->w / 2.0 * scale), center_point.y - height / 2, (int)(surf->w * scale), (int)(surf->h * scale)};

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surf);
    if (!texture)
    {
        SDL_FreeSurface(surf);
        return false;
    }
    SDL_Texture *former_texture = SDL_GetRenderTarget(renderer);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, texture, srcrect, (dstrect == NULL ? &rect : dstrect));

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(texture);
    SDL_SetRenderTarget(renderer, former_texture);
    former_texture = NULL;
    return true;
}

//----==== OK! Tested ====-----
SDL_Rect render_text_center(SDL_Renderer *renderer, const char *text, SDL_Point *center_point, TTF_Font *font = NULL, SDL_Color color = {0, 0, 0, 255})
{
    string t = text;
    SDL_Texture *pervious_target = SDL_GetRenderTarget(renderer);
    if (font == NULL)
        font = TTF_OpenFont("./arial.ttf", 20);
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect result = {center_point->x - surf->w / 2, center_point->y - surf->h / 2, surf->w, surf->h};

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, txt, NULL, &result);

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(txt);
    SDL_SetRenderTarget(renderer, pervious_target);

    return result;
}

SDL_Rect render_text_right(SDL_Renderer *renderer, const char *text, SDL_Point *right_center_point, TTF_Font *font = TTF_OpenFont(FONT_ADDR, 20), SDL_Color color = {0, 0, 0, 255})
{
    SDL_Texture *pervious_target = SDL_GetRenderTarget(renderer);
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect result = {right_center_point->x - surf->w, right_center_point->y - surf->h / 2, surf->w, surf->h};

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, txt, NULL, &result);

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(txt);
    SDL_SetRenderTarget(renderer, pervious_target);

    return result;
}

SDL_Rect render_text_left(SDL_Renderer *renderer, const char *text, SDL_Point *left_center_point, TTF_Font *font = TTF_OpenFont(FONT_ADDR, 20), SDL_Color color = {0, 0, 0, 255})
{
    SDL_Texture *pervious_target = SDL_GetRenderTarget(renderer);
    SDL_Surface *surf = TTF_RenderText_Blended(font, text, color);
    SDL_Texture *txt = SDL_CreateTextureFromSurface(renderer, surf);
    SDL_Rect result = {left_center_point->x, left_center_point->y - surf->h / 2, surf->w, surf->h};

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderCopy(renderer, txt, NULL, &result);

    SDL_FreeSurface(surf);
    SDL_DestroyTexture(txt);
    SDL_SetRenderTarget(renderer, pervious_target);

    return result;
}

int check_for_collision(SDL_Rect first, SDL_Rect second)
{
    if (SDL_HasIntersection(&first, &second))
    {
        if (second.x - first.x > 0 && second.x - first.x < first.w)
            return 1;
        if (first.x - second.x > 0 && first.x - second.x < second.w)
            return 3;

        if (second.y - first.y > 0 && second.y - first.y < first.h)
            return 0;
        if (first.y - second.y > 0 && first.y - second.y < second.h)
        {
            return 2;
        }
    }
    return -1;
}
typedef enum States
{
    STATE_START_MENU,
    STATE_PAUSE_MENU,
    STATE_END_MENU,
    STATE_GAMING,
    STATE_QUIT
} States;
States Game_State = STATE_START_MENU;

typedef enum Powers
{
    KICKFIRE,
    PUNCH,
    INVISIBLE_BALL,
    THIEF,
    NONE
} Powers;
Powers Power;

typedef struct Ball
{
private:
    int vx = 0;
    int vy = 0;
    Uint8 ay = GRAVITY;

    Powers power = NONE;
    string power_owner = "";
    Uint32 start_invisibility_time = 0;

    const int MAX_VELOCITY = 55;

    int current_number_cntr = 0;
    const Uint8 pictures_cnt = 2;
    Uint8 current_number = 0;
    Uint8 current_model = 0;
    const char *ball_root = "./raw/Balls/";

    SDL_Point *pcenter = new SDL_Point{0, 0};
    int r = 0;
    int initial_r = 0;
    SDL_Color color{0, 0, 0, 255};

    string create_addres()
    {
        string result = "";
        result += string(ball_root);
        result += to_string(current_model);
        result += (current_number > 0 ? string("(") + to_string(current_number) + string(")") : "");
        result += string(".png");
        return result;
    }

public:
    Ball(SDL_Point *pcenter, Uint8 vx, Uint16 r, SDL_Color color, int model)
    {
        this->vx = vx;
        this->pcenter = pcenter;
        this->r = r;
        this->color = color;
        this->current_model = model;
        initial_r = r;
    }
    void set_power(Powers new_power, string power_owner) { power = new_power, this->power_owner = power_owner; }
    Powers get_power() { return power; }
    void set_model(int new_model)
    {
        current_model = new_model;
    }
    void render(SDL_Renderer *renderer)
    {
        int x = pcenter->x;
        int y = pcenter->y;
        if (vx > MAX_VELOCITY)
        {
            vx = MAX_VELOCITY;
        }
        if (vy > MAX_VELOCITY || vy < -MAX_VELOCITY)
        {
            // vy = MAX_VELOCITY * (vy > 0 ? 1 : -1);
        }
        vy += GRAVITY;
        pcenter->x += vx;
        pcenter->y += vy;
        if (pcenter->y + r > HEIGHT - BOTTOM_MARGIN)
        {
            vy *= -0.9;
            pcenter->y = HEIGHT - BOTTOM_MARGIN - r;
        }
        if (pcenter->x < r)
        {
            vx *= -1;
            pcenter->x = r;
        }
        if (pcenter->x + r > WIDTH)
        {
            vx *= -1;
            pcenter->x = WIDTH - r;
        }
        current_number_cntr++;
        if (current_number_cntr >= pictures_cnt)
        {
            current_number++;
            current_number_cntr = 0;
            if (current_number >= pictures_cnt)
            {
                current_number = 0;
            }
        }
        if (vy < -1000)
        {
            vy = -1000;
        }
        if (vx == 0)
        {
            current_number = 0;
        }
        if (power != INVISIBLE_BALL)
        {
            if (power == KICKFIRE)
                r = 1.5 * initial_r;
            if (power == NONE)
            {
                r = initial_r;
            }
            draw_image_on_point(renderer, *pcenter, 2 * r, create_addres().c_str());
        }
        else
        {
            if (start_invisibility_time == 0)
            {
                start_invisibility_time = SDL_GetTicks();
            }
            if (SDL_GetTicks() - start_invisibility_time > 3000)
            {
                start_invisibility_time = 0;
                power = NONE;
            }
        }
    }
    void set_vx(int vx) { this->vx = vx; }
    void set_vy(int vy) { this->vy = vy; }
    int get_ay() { return ay; }
    int get_vx() { return vx; }
    int get_vy() { return vy; }
    string get_power_owner() { return power_owner; }
    SDL_Point get_center()
    {
        return *pcenter;
    }
    void set_x(Sint16 x)
    {
        pcenter->x = x;
    }
    void set_y(Sint16 y)
    {
        pcenter->y = y;
    }
    void set_center(SDL_Point new_center)
    {
        *pcenter = new_center;
    }

    Sint16 get_x() { return pcenter->x; }
    Sint16 get_y() { return pcenter->y; }

    Uint16 get_r() { return r; }
    SDL_Rect get_bounds()
    {
        return (SDL_Rect{pcenter->x - r, pcenter->y - r, 2 * r, 2 * r});
    }
} Ball;

typedef enum Char_modes
{
    NORMAL,
    CONFUSED,
    FREEZED,
    TRIPLE,
    RUNNING_LEFT,
    RUNNING_RIGHT,
    JUMPING
} Char_modes;

typedef enum Char_types
{
    CHARACTER_RIGHT,
    CHARACTER_LEFT
} Char_types;

typedef struct Character
{
private:
    Ball *ball = NULL;
    Char_types type = CHARACTER_RIGHT;
    Char_modes mode = NORMAL;
    int body_number = 0;
    int head_number = 0;

    Uint8 num_of_goals = 0;

    int power_percent = 0;
    int initial_y = 0;

    int shoes_current_number = 0;
    int shoes_model = 0;
    const static int shoes_cnt = 4;
    const static int conf_pics_cnt = 2;

    int current_conf_pic = 0;

    const char *head_root = "./raw/Char/heads/";
    const char *body_root = "./raw/Char/bodies/";
    const char *shoes_root = "./raw/Char/shoes/";
    const char *conf_pics_root = "./raw/Char/confusing/";

    Uint32 conf_time = 0;

    float head_to_height_ratio = 0.5;
    float body_to_height_ratio = 0.25;
    float shoes_to_height_ratio = 0.25;

    const int triple_margin = 30;
    const int x_speed = 20;
    const int y_speed = 75;
    int dx = 0, dy = 0, dvy = 0;
    int keys[4] = {SDLK_RIGHT, SDLK_LEFT, SDLK_UP, SDLK_DOWN};

    string name = " ";
    Powers power = NONE;

    SDL_Event *event = NULL;

    SDL_Rect bounds{0, 0, 0, 0};
    SDL_Texture *back_texture = NULL;
    SDL_Texture *head_texture = NULL;
    SDL_Texture *body_texture = NULL;
    SDL_Texture *shoes_textures[shoes_cnt] = {NULL};
    SDL_Rect body_rect{0, 0, 0, 0};
    SDL_Rect head_rect{0, 0, 0, 0};
    SDL_Rect shoes_rects[shoes_cnt];

    void set_scales()
    {
        int h = bounds.h;
        float head_ratio = (h * head_to_height_ratio) / head_rect.h;
        head_rect.h *= head_ratio;
        head_rect.w *= head_ratio;

        float body_ratio = (h * body_to_height_ratio) / body_rect.h;
        body_rect.h *= body_ratio;
        body_rect.w *= body_ratio;
        body_rect.y = bounds.y + head_rect.h;

        float shoes_ratio = (h * shoes_to_height_ratio) / shoes_rects[0].h;

        for (int i = 0; i < shoes_cnt; i++)
        {
            shoes_rects[i].h *= shoes_ratio;
            shoes_rects[i].w *= shoes_ratio;
            shoes_rects[i].y = body_rect.y + body_rect.h;
        }
    }
    void fill_textures(SDL_Renderer *renderer)
    {
        SDL_Surface *body_img = IMG_Load(create_body_and_head_address(body_root, body_number).c_str());
        body_texture = SDL_CreateTextureFromSurface(renderer, body_img);
        body_rect = body_img->clip_rect;
        SDL_FreeSurface(body_img);

        SDL_Surface *head_img = IMG_Load(create_body_and_head_address(head_root, head_number).c_str());
        head_texture = SDL_CreateTextureFromSurface(renderer, head_img);
        head_rect = head_img->clip_rect;
        SDL_FreeSurface(head_img);

        for (int i = 0; i < shoes_cnt; i++)
        {
            SDL_Surface *img = IMG_Load(create_shoes_address(shoes_model, i).c_str());
            shoes_textures[i] = SDL_CreateTextureFromSurface(renderer, img);
            shoes_rects[i] = img->clip_rect;
            // float scale = (float)(bounds.h) / (float)(img->h);
            // pics_bounds[i].w *= scale;
            // pics_bounds[i].h *= scale;
            SDL_FreeSurface(img);
        }
    }
    void destroy_pics_textures()
    {
        SDL_DestroyTexture(head_texture);
        head_rect = {0, 0, 0, 0};
        head_texture = NULL;
        SDL_DestroyTexture(body_texture);
        body_rect = {0, 0, 0, 0};
        body_texture = NULL;
        for (int i = 0; i < shoes_cnt; i++)
        {
            SDL_DestroyTexture(shoes_textures[i]);
            shoes_rects[i] = {0, 0, 0, 0};
            shoes_textures[i] = NULL;
        }
    }
    void render_head(SDL_Renderer *renderer)
    {
        SDL_Point dstpoint = {bounds.x + bounds.w / 2, bounds.y + head_rect.h / 2};
        SDL_RenderCopy(renderer, head_texture, NULL, &head_rect);
    }
    void render_body(SDL_Renderer *renderer)
    {
        SDL_Point dstpoint = {bounds.x + bounds.w / 2, bounds.y + head_rect.h + body_rect.h / 2};
        SDL_RenderCopy(renderer, body_texture, NULL, &body_rect);
    }
    void render_foot(SDL_Renderer *renderer)
    {
        SDL_Point dstpoint{0, 0};
        dstpoint.x = bounds.x + bounds.w / 2;
        dstpoint.y = bounds.y + head_rect.h + body_rect.h + shoes_rects[shoes_current_number].h / 2;
        SDL_RenderCopy(renderer, shoes_textures[shoes_current_number], NULL, &shoes_rects[shoes_current_number]);
    }
    string create_shoes_address(int model, int number)
    {
        string result = shoes_root;
        result += to_string(model);
        if (number > 0)
        {
            result += "(";
            result += to_string(number);
            result += ")";
        }
        result += ".png";
        return result;
    }
    string create_body_and_head_address(string root, int pic_number)
    {
        root += to_string(pic_number);
        root += ".png";
        return root;
    }
    string create_conf_pic_addr()
    {
        string result = string(conf_pics_root);
        current_conf_pic++;
        if (current_conf_pic >= 10)
        {
            current_conf_pic = 0;
        }
        result += to_string(current_conf_pic / 5);
        result += ".png";
        return result;
    }
    void set_mode()
    {
        const int right = keys[0];
        const int left = keys[1];
        const int up = keys[2];
        const int power_k = keys[3];

        switch (event->type)
        {
        case SDL_KEYDOWN:
        {
            int key = event->key.keysym.sym;

            if (key == up)
            {
                mode = JUMPING;
            }
            if (key == left)
            {
                mode = RUNNING_LEFT;
            }
            if (key == right)
            {
                mode = RUNNING_RIGHT;
            }
            if (key == power_k)
            {
                SDL_Rect &r = shoes_rects[shoes_current_number];
                if (power_percent >= 100 && SDL_PointInRect(new SDL_Point{ball->get_x(), ball->get_y()}, new SDL_Rect{r.x - 50, r.y - 50, r.w + 100, r.h + 100}))
                {
                    ball->set_power(power, name);
                    power_percent = 0;
                }
            }
        }
        break;

        case SDL_KEYUP:
        {
            int key = event->key.keysym.sym;
            if (key == left)
            {
                mode = NORMAL;
                break;
            }
            if (key == right)
            {
                mode = NORMAL;
                break;
            }
        }
        break;
        }
    }
    bool ball_head_colision()
    {
        SDL_Rect r = this->get_head_rect();
        int collision = check_for_collision(r, ball->get_bounds());
        if (collision >= 0)
        {
            ball->set_vx(dx - ball->get_vx());
            if (collision == 1)
            {
                ball->set_x(r.x + r.w + ball->get_r());
            }
            if (collision == 3)
            {
                ball->set_x(r.x - ball->get_r());
            }
            if (collision == 0)
            {
                ball->set_y(r.y - ball->get_r());
            }
            if (collision == 2)
            {
                ball->set_y(r.y + ball->get_r());
            }
            ball->set_vy(dy - ball->get_vy());
            if (name != ball->get_power_owner())
            {
                switch (ball->get_power())
                {
                case KICKFIRE:
                    bounds.x = bounds.x + (type == CHARACTER_LEFT ? -100 : 100);
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                case PUNCH:
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                default:
                    break;
                }
            }
            return true;
        }
        return false;
    }
    bool ball_body_collision()
    {
        SDL_Rect rect = this->get_body_rect();
        int collision = check_for_collision(rect, ball->get_bounds());
        if (collision >= 0)
        {
            ball->set_vx(dx - ball->get_vx() * 0.92);
            if (collision == 1)
            {
                ball->set_x(rect.x + rect.w + ball->get_r());
            }
            if (collision == 3)
            {
                ball->set_x(rect.x - ball->get_r());
            }
            if (collision == 0)
            {
                ball->set_y(rect.y - ball->get_r());
            }
            if (collision == 2)
            {
                ball->set_y(rect.y + ball->get_r());
            }
            ball->set_vy(dy - ball->get_vy() * 0.92);
            if (name != ball->get_power_owner())
            {
                switch (ball->get_power())
                {
                case KICKFIRE:
                    bounds.x = bounds.x + (type == CHARACTER_LEFT ? -100 : 100);
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                case PUNCH:
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                default:
                    break;
                }
            }
            return true;
        }
        return false;
    }
    bool ball_foot_collision()
    {
        SDL_Rect r = this->get_feet_rect();
        int collision = check_for_collision(r, ball->get_bounds());
        if (collision >= 0)
        {
            if (collision == 1)
            {
                if (type == CHARACTER_LEFT)
                {
                    ball->set_vx(dx - ball->get_vx() * 0.95);
                    ball->set_vy(dy + 30);
                    ball->set_x(r.x + r.w + ball->get_r());
                }
            }
            if (collision == 3)
            {
                if (type == CHARACTER_RIGHT)
                {
                    ball->set_vx(dx - ball->get_vx() * 0.95);
                    ball->set_vy(dy + 30);
                    ball->set_x(r.x - ball->get_r() - 5);
                }
            }
            if (collision == 0)
            {
                ball->set_vx(dx - (ball->get_vx() + ball->get_vy()) / 2);
                ball->set_vy(dx - (ball->get_vx() + ball->get_vy()) / 2);
                ball->set_y(r.y - ball->get_r());
            }
            if (collision == 2)
            {
                ball->set_vx(dx + (ball->get_vx() + ball->get_vy()) / 2);
                ball->set_vy(dx - (ball->get_vx() + ball->get_vy()) / 2);
                ball->set_y(r.y + ball->get_r());
            }
            if (name != ball->get_power_owner())
            {
                switch (ball->get_power())
                {
                case KICKFIRE:
                    bounds.x = bounds.x + (type == CHARACTER_LEFT ? -100 : 100);
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                case PUNCH:
                    mode = CONFUSED;
                    ball->set_power(NONE, name);
                    break;
                default:
                    break;
                }
            }
            return true;
        }

        return false;
    }

public:
    Character(SDL_Renderer *renderer, SDL_Event *event, SDL_Rect bounds, Char_types type, Ball *ball, Powers power, int head_number = 0, int body_number = 0, int shoes_model = 0, int shoe_number = 0)
    {
        this->head_number = head_number;
        this->body_number = body_number;
        this->shoes_model = shoes_model;
        this->shoes_current_number = shoe_number;
        this->bounds = bounds;
        this->event = event;
        fill_textures(renderer);
        set_scales();
        initial_y = bounds.y;
        this->ball = ball;
        this->type = type;
        this->power = power;
    }
    Character(Character &character) = delete;
    void set_keys(int right, int left, int up, int power_key) { keys[0] = right, keys[1] = left, keys[2] = up, keys[3] = power_key; }
    Uint8 get_num_of_goals() { return num_of_goals; }
    void add_goal() { num_of_goals++; }
    Char_modes get_mode() { return mode; }
    Char_types get_type() { return type; }
    void set_name(string name) { this->name = name; }
    bool ball_collision(SDL_Rect *ball_bounds)
    {
        if (ball_bounds->y > bounds.y)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    void set_texture(SDL_Texture *new_texture) { back_texture = new_texture; }
    SDL_Texture *get_texture() { return back_texture; }
    void render(SDL_Renderer *renderer)
    {
        SDL_SetRenderTarget(renderer, back_texture);
        if (mode != CONFUSED)
        {
            set_mode();
        }
        else
        {
            dx = 0;
            dy = 0;
        }
        render_body(renderer);
        render_foot(renderer);
        render_head(renderer);
        if (mode == RUNNING_LEFT || mode == RUNNING_RIGHT)
        {
            dx = (mode == RUNNING_LEFT ? -x_speed : x_speed);
            shoes_current_number++;
            if (shoes_current_number >= shoes_cnt)
            {
                shoes_current_number = 0;
            }
        }
        if (mode == NORMAL)
        {
            shoes_current_number = 0;
            dx = 0;
        }
        if (mode == JUMPING)
        {
            if (dvy == 0)
            {
                dvy = GRAVITY;
                dy = -y_speed;
            }
        }
        if (bounds.y > initial_y)
        {
            bounds.y = initial_y;
            dy = 0;
            dvy = 0;
            mode = NORMAL;
        }
        bounds.x += dx;
        dy += dvy;
        bounds.y += dy;
        head_rect = this->get_head_rect();
        body_rect = this->get_body_rect();
        shoes_rects[shoes_current_number] = this->get_feet_rect();

        if (mode != CONFUSED)
        {
            if (!ball_foot_collision())
            {
                if (!ball_head_colision())
                {
                    ball_body_collision();
                }
            }
        }
        else
        {
            if (conf_time == 0)
            {
                conf_time = SDL_GetTicks();
            }
            if (SDL_GetTicks() - conf_time > 3000)
            {
                mode = NORMAL;
                conf_time = 0;
            }
            SDL_RenderCopy(renderer, IMG_LoadTexture(renderer, create_conf_pic_addr().c_str()), NULL, new SDL_Rect{bounds.x - 20, bounds.y - 100, head_rect.w + 40, 100});
        }
        if (bounds.x < 0)
        {
            bounds.x = 1;
        }
        if (bounds.x + bounds.w > WIDTH)
        {
            bounds.x = WIDTH - bounds.w;
        }
        power_percent += 1;
    }
    int get_vx() { return dx; }
    int get_vy() { return dy; }
    SDL_Rect get_head_rect()
    {
        return SDL_Rect{bounds.x + (bounds.w - head_rect.w) / 2, bounds.y, head_rect.w, head_rect.h};
    }
    SDL_Rect get_body_rect()
    {
        return SDL_Rect{bounds.x + (bounds.w - body_rect.w) / 2, bounds.y + head_rect.h, body_rect.w, body_rect.h};
    }
    SDL_Rect get_feet_rect()
    {
        SDL_Rect r = shoes_rects[shoes_current_number];
        r.x = bounds.x + (bounds.w - shoes_rects[shoes_current_number].w) / 2;
        r.y = bounds.y + head_rect.h + body_rect.h;
        return r;
    }
    string get_name() { return name; }
    void set_power_precent(int p)
    {
        power_percent = p;
        if (power_percent > 100)
            power_percent = 100;
    }
    int get_power_precent() { return power_percent; }
    void set_vx(int new_vx) { dx = new_vx; }
    void set_vy(int new_vy) { dy = new_vy; }
    ~Character()
    {
        SDL_DestroyTexture(back_texture);
        back_texture = NULL;
        destroy_pics_textures();
    }
} Character;

typedef struct ProgressBar
{
private:
    Uint16 total_value = 0;
    Uint16 current_value = 0;
    SDL_Color front_color;
    SDL_Color back_color;
    SDL_Rect bounds;
    SDL_Texture *back_texture = NULL;
    TTF_Font *font = TTF_OpenFont("./arial.ttf", 20);

public:
    ProgressBar(Uint16 total_value, Uint16 current_value, SDL_Color front_color, SDL_Color back_color, SDL_Rect bounds)
    {
        this->total_value = total_value;
        this->current_value = current_value;
        this->front_color = front_color;
        this->back_color = back_color;
        this->bounds = bounds;
    }

    void set_value(Uint16 value)
    {
        if (value > total_value)
        {
            value = total_value;
        }
        current_value = value;
    }

    Uint16 get_value()
    {
        return current_value;
    }

    void render(SDL_Renderer *renderer)
    {
        const float margin_ratio = 0.1;
        const int margin = 5;
        float ratio = (float)current_value / (float)total_value;
        SDL_Texture *former = SDL_GetRenderTarget(renderer);
        SDL_Rect front_rect{bounds.x + margin, bounds.y + margin, int(bounds.w * ratio * ((float)(bounds.w - 2 * margin) / (float)bounds.w)), bounds.h - 2 * margin};

        if (back_texture == NULL)
            back_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_RENDERER_TARGETTEXTURE, bounds.w, bounds.h);
        SDL_SetRenderTarget(renderer, back_texture);
        SDL_SetRenderDrawColor(renderer, back_color.r, back_color.g, back_color.b, back_color.a);
        SDL_RenderFillRect(renderer, &bounds);
        SDL_SetRenderDrawColor(renderer, front_color.r, front_color.g, front_color.b, front_color.a);
        SDL_RenderFillRect(renderer, &front_rect);
        SDL_SetRenderTarget(renderer, NULL);
        render_text_center(renderer, (to_string((int)(ratio * 100.0)) + string("%")).c_str(), new SDL_Point{bounds.x + bounds.w / 2, bounds.y + bounds.h / 2}, font);
        SDL_RenderCopy(renderer, back_texture, &bounds, &bounds);
        SDL_SetRenderTarget(renderer, former);
    }
    ~ProgressBar()
    {
        SDL_DestroyTexture(back_texture);
        back_texture = NULL;
    }
} ProgressBar;

typedef struct Timer
{
private:
    Uint64 start_time = 0;
    Uint64 total_time = 0;
    Uint16 alarm = 0;
    bool is_started = false;
    bool is_on = false;
    Timer(Timer &timer) = delete;

public:
    Timer()
    {
        start_time = time(nullptr);
        is_on = true;
        is_started = true;
    }
    void play()
    {
        is_on = true;
    }
    void pause()
    {
        if (is_on)
        {
            total_time = time(nullptr) - start_time;
        }
        start_time = time(nullptr) - total_time;
        is_on = false;
    }
    Uint64 get_time()
    {
        if (is_on)
        {
            return time(nullptr) - start_time;
        }
        return total_time;
    }
    void set_alarm(Uint16 time_in_seconds)
    {
        alarm = time_in_seconds;
    }

    bool get_status()
    {
        return is_on;
    }

    void clear()
    {
        total_time = 0;
        start_time = time(nullptr);
    }

    bool check_alarm()
    {
        return get_time() > alarm;
    }
} Timer;

typedef struct Text
{
    std::string text = " ";
    SDL_Color color = {0, 0, 0, 255};
    int ptsize = 20;
    std::string font_addr = FONT_ADDR;
    TTF_Font *font = TTF_OpenFont(font_addr.c_str(), ptsize);

    void update_font()
    {
        font = TTF_OpenFont(font_addr.c_str(), ptsize);
    }

    SDL_Rect render(SDL_Renderer *renderer, SDL_Point *center_point)
    {
        return render_text_center(renderer, text.c_str(), center_point, font, color);
    }

} Text;

typedef struct TextBox
{
private:
    Text *ptext;
    SDL_Rect bounds;
    SDL_Texture *back_texture = NULL;
    SDL_Color back_color{255, 255, 200, 255};
    SDL_Color border_color{255, 100, 120, 255};
    bool enabled = false;
    SDL_Event *e = NULL;

    void read_keys_and_mouse(SDL_Event *event)
    {
        int x = 0, y = 0;
        if (SDL_GetMouseState(&x, &y) & SDL_BUTTON_LMASK)
        {
            if (SDL_PointInRect(new SDL_Point{x, y}, &bounds))
            {
                enabled = true;
            }
            else
            {
                enabled = false;
            }
        }

        if (enabled)
        {
            if (event->type == SDL_KEYDOWN)
            {
                switch (event->key.keysym.sym)
                {
                case SDLK_BACKSPACE:
                {
                    if (ptext->text.size() > 0)
                        ptext->text.erase(ptext->text.end() - 1);
                    if (ptext->text.size() < 1)
                    {
                        ptext->text = " ";
                    }
                    break;
                }
                default:
                {
                    if (isprint(event->key.keysym.sym))
                        ptext->text += event->key.keysym.sym;
                }
                }
            }
        }
    }

public:
    TextBox(Text *text, SDL_Color back_color, SDL_Rect bounds, SDL_Event *e)
    {
        if (text == NULL)
        {
            ptext = new Text;
        }
        else
        {
            this->ptext = text;
        }

        ptext->text = " ";
        this->back_color = back_color;
        this->bounds = bounds;
        this->e = e;
    }
    string get_text() { return ptext->text; }
    void render(SDL_Renderer *renderer)
    {
        read_keys_and_mouse(e);
        // SDL_Texture *former_texture = SDL_GetRenderTarget(renderer);
        if (back_texture == NULL)
        {
            back_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, bounds.w, bounds.h);
        }
        SDL_SetRenderTarget(renderer, back_texture);
        SDL_SetRenderDrawColor(renderer, back_color.r, back_color.g, back_color.b, back_color.a);
        SDL_RenderFillRect(renderer, new SDL_Rect{0, 0, bounds.w, bounds.h});

        if (enabled)
        {
            SDL_SetRenderDrawColor(renderer, border_color.r, border_color.g, border_color.b, border_color.a);
            SDL_RenderDrawRect(renderer, new SDL_Rect{0, 0, bounds.w, bounds.h});
        }
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, back_texture, NULL, &bounds);
        // if (former_texture != NULL)
        // {
        //     SDL_SetRenderTarget(renderer, former_texture);
        // }
        render_text_center(renderer, ptext->text.c_str(), new SDL_Point{bounds.x + bounds.w / 2, bounds.y + bounds.h / 2});
    }
} TextBox;

struct Button
{
private:
    SDL_Rect bounds;
    Text text;
    SDL_Texture *back_texture = NULL;
    SDL_Color back_color{0, 0, 0, 0};

    void init_text()
    {
        text.text = " ";
    }

public:
    Button(SDL_Renderer *p_renderer, SDL_Surface *back, SDL_Rect bounds)
    {
        init_text();
        this->bounds.x = bounds.x;
        this->bounds.y = bounds.y;
        this->bounds.w = bounds.w;
        this->bounds.h = bounds.h;
        back_texture = SDL_CreateTexture(p_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, bounds.w, bounds.h);
    }
    Button(SDL_Renderer *p_renderer, SDL_Color back_color, SDL_Rect bounds)
    {
        init_text();
        this->bounds.x = bounds.x;
        this->bounds.y = bounds.y;
        this->bounds.w = bounds.w;
        this->bounds.h = bounds.h;
        back_texture = SDL_CreateTexture(p_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, bounds.w, bounds.h);

        SDL_Texture *former_texture = SDL_GetRenderTarget(p_renderer);

        SDL_SetRenderTarget(p_renderer, back_texture);
        SDL_SetRenderDrawColor(p_renderer, back_color.r, back_color.g, back_color.b, back_color.a);
        SDL_RenderClear(p_renderer);
        SDL_SetRenderTarget(p_renderer, NULL);
        SDL_RenderCopy(p_renderer, back_texture, NULL, &bounds);
        if (former_texture != NULL)
            SDL_SetRenderTarget(p_renderer, former_texture);
        former_texture = NULL;
    }

    Button(SDL_Renderer *p_renderer, Uint8 back_r, Uint8 back_g, Uint8 back_b, Uint8 back_a, SDL_Rect bounds)
    {
        init_text();
        SDL_Color back_color{back_r, back_g, back_b, back_a};
        this->bounds.x = bounds.x;
        this->bounds.y = bounds.y;
        this->bounds.w = bounds.w;
        this->bounds.h = bounds.h;
        back_texture = SDL_CreateTexture(p_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, bounds.w, bounds.h);

        SDL_Texture *former_texture = SDL_GetRenderTarget(p_renderer);

        SDL_SetRenderTarget(p_renderer, back_texture);
        SDL_SetRenderDrawColor(p_renderer, back_color.r, back_color.g, back_color.b, back_color.a);
        SDL_RenderClear(p_renderer);
        SDL_SetRenderTarget(p_renderer, NULL);
        SDL_RenderCopy(p_renderer, back_texture, NULL, &bounds);
        if (former_texture != NULL)
            SDL_SetRenderTarget(p_renderer, former_texture);
        former_texture = NULL;
    }
    Button(SDL_Renderer *p_renderer, std::string back_addr, SDL_Rect bounds)
    {
        init_text();
        SDL_Surface *back_surf = IMG_Load(back_addr.c_str());
        back_texture = SDL_CreateTextureFromSurface(p_renderer, back_surf);
        SDL_FreeSurface(back_surf);
        this->bounds.x = bounds.x;
        this->bounds.y = bounds.y;
        this->bounds.w = bounds.w;
        this->bounds.h = bounds.h;
    }
    void set_font(const char *font_addr)
    {
        text.font_addr = font_addr;
        text.update_font();
    }
    void set_text_size(int size)
    {
        text.ptsize = size;
        text.update_font();
    }
    void set_font_and_size(const char *font_addr, int size)
    {
        text.font_addr = font_addr;
        text.ptsize = size;
        text.update_font();
    }
    bool is_clicked()
    {
        Uint32 state;
        int x = 0, y = 0;
        state = SDL_GetMouseState(&x, &y);
        SDL_PumpEvents();
        return (SDL_PointInRect(new SDL_Point{x, y}, &bounds) && (state & SDL_BUTTON_LMASK));
    }
    void set_text(string text)
    {
        this->text.text = text;
    }
    void render(SDL_Renderer *renderer, SDL_Rect *srcrect = NULL)
    {
        int x = 0, y = 0;
        SDL_GetMouseState(&x, &y);
        SDL_Texture *former_texture = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, back_texture, srcrect, &bounds);

        if (SDL_PointInRect(new SDL_Point{x, y}, &bounds))
        {
            SDL_SetRenderDrawColor(renderer, 250, 100, 0, 255);
            SDL_RenderDrawRect(renderer, &bounds);
        }
        text.render(renderer, new SDL_Point{bounds.w / 2 + bounds.x, bounds.h / 2 + bounds.y});
        SDL_SetRenderTarget(renderer, former_texture);
        former_texture = NULL;
    }
    SDL_Color get_color()
    {
        return back_color;
    }
    void set_back_color(SDL_Renderer *renderer, SDL_Color color)
    {
        back_color = color;
        SDL_Texture *former = SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer, back_texture);
        SDL_SetRenderDrawColor(renderer, back_color.r, back_color.g, back_color.b, back_color.a);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, back_texture, NULL, &bounds);
        SDL_SetRenderTarget(renderer, former);
    }
    ~Button()
    {
        SDL_DestroyTexture(back_texture);
    }
};

typedef struct Button Button;

void clear_window(SDL_Renderer *m_renderer)
{
    SDL_SetRenderTarget(m_renderer, NULL);
    SDL_SetRenderDrawColor(m_renderer, 30, 40, 50, 255);
    SDL_RenderClear(m_renderer);
}

void window_stuff(SDL_Renderer *m_renderer, SDL_Event *e)
{
    SDL_PollEvent(e);
    if (e->type == SDL_QUIT)
    {
        Game_State = STATE_QUIT;
    }
    SDL_SetRenderTarget(m_renderer, NULL);
    SDL_RenderPresent(m_renderer);
    SDL_Delay(DELAY);
}

// bool read_games_from_file(vector<Game> &character)
// {
//     std::ifstream file(DATA_FILE_ADDR);
//     if (!file.is_open())
//     {
//         return false;
//     }
// }

bool write_game_to_file(string data)
{
    ofstream out(DATA_FILE_ADDR);
    if (!out.is_open())
    {
        system("mkdir data");
        system("mkdir data/games");
        system("touch ./data/games/data");
        out.open(DATA_FILE_ADDR);
        if (!out.is_open())
        {
            return false;
        }
    }
    // data += "\n";
    out << data;
    out.close();
    return true;
}

int main(int argc, char *argv[])
{
    SDL_Window *m_window = NULL;
    SDL_Renderer *m_renderer = NULL;
    // Initialization of SDL window
    {
        Uint32 SDL_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
        Uint32 WND_flags = SDL_WINDOW_HIDDEN;

        if (SDL_Init(SDL_flags) < 0 || TTF_Init() < 0 || IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) == 0)
        {
            printf("Error with SDL_Init: %s", SDL_GetError());
            return -1;
        }

        SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, WND_flags, &m_window, &m_renderer);
        if (!m_renderer || !m_window)
        {
            printf("Error with renderer or window: %s", SDL_GetError());
            return -1;
        }

        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_ShowWindow(m_window);
        SDL_RenderClear(m_renderer);
        SDL_RenderPresent(m_renderer);
    }

    SDL_Event *e = new SDL_Event;
    srand(time(nullptr));
    string l_char_name = "";
    string r_char_name = "";
    //-----====-Main game loop start-====-----
    while (1)
    {
        window_stuff(m_renderer, e);

        if (Game_State == STATE_START_MENU)
        {
            // Initialization

            Button btn_next(m_renderer, SDL_Color{140, 240, 150, 255}, SDL_Rect{WIDTH / 2 - 100, 600, 200, 60});
            TextBox r_ch_tb(NULL, SDL_Color{220, 200, 180, 255}, {800, 200, 200, 80}, e);
            TextBox l_ch_tb(NULL, SDL_Color{220, 200, 180, 255}, {200, 200, 200, 80}, e);
            btn_next.set_text("Next");

            // Loop
            while (Game_State == STATE_START_MENU)
            {
                clear_window(m_renderer);

                btn_next.render(m_renderer);
                r_ch_tb.render(m_renderer);
                l_ch_tb.render(m_renderer);

                if (btn_next.is_clicked())
                {
                    Game_State = STATE_GAMING;
                    l_char_name = l_ch_tb.get_text();
                    r_char_name = r_ch_tb.get_text();
                }
                window_stuff(m_renderer, e);
            }
        }
        if (Game_State == STATE_GAMING)
        {
            // initialization
            SDL_Point ball_center{WIDTH / 2, HEIGHT - BOTTOM_MARGIN};
            Ball ball(&ball_center, 20, 30, {240, 180, 200, 255}, 1);

            Character l_char(m_renderer, e, {100, HEIGHT - BOTTOM_MARGIN - CHAR_HEIGHT, 100, CHAR_HEIGHT}, CHARACTER_LEFT, &ball, static_cast<Powers>(rand() % 3), 0, 0, 3);
            Character r_char(m_renderer, e, {WIDTH - 100 - 100, HEIGHT - BOTTOM_MARGIN - CHAR_HEIGHT, 100, CHAR_HEIGHT}, CHARACTER_RIGHT, &ball, static_cast<Powers>(rand() % 3), 0, 1, 2);
            l_char.set_keys(SDLK_d, SDLK_a, SDLK_w, SDLK_s);
            l_char.set_name(l_char_name);
            r_char.set_name(r_char_name);
            ProgressBar power_r(100, 0, {100, 200, 200, 255}, {240, 240, 255, 255}, {WIDTH - 300, HEIGHT - 75, 200, 30});
            ProgressBar power_l(100, 0, {100, 200, 200, 255}, {240, 240, 255, 255}, {100, HEIGHT - 75, 200, 30});
            Timer game_timer;
            game_timer.pause();
            // loop
            while (Game_State == STATE_GAMING)
            {
                clear_window(m_renderer);
                game_timer.play();
                r_char.render(m_renderer);
                l_char.render(m_renderer);
                ball.render(m_renderer);
                power_l.render(m_renderer);
                power_r.render(m_renderer);
                power_r.set_value(r_char.get_power_precent());
                power_l.set_value(l_char.get_power_precent());

                // Check for goals
                if (ball_center.x > WIDTH - 100 || ball_center.x < 100)
                {
                    if (ball_center.x > WIDTH - 200)
                    {
                        l_char.add_goal();
                        r_char.set_power_precent(r_char.get_power_precent() + 20);
                    }
                    else
                    {
                        r_char.add_goal();
                        l_char.set_power_precent(l_char.get_power_precent() + 20);
                    }
                    ball_center.x = WIDTH / 2;
                    ball.set_vy(0);
                    ball.set_vx(0);
                    ball_center.y = HEIGHT - BOTTOM_MARGIN;
                }
                // Display names and goals and power bar
                render_text_center(m_renderer, l_char.get_name().c_str(), new SDL_Point{150, 50}, TTF_OpenFont(FONT_ADDR, 50), {255, 255, 220, 255});
                render_text_center(m_renderer, r_char.get_name().c_str(), new SDL_Point{WIDTH - 150, 50}, TTF_OpenFont(FONT_ADDR, 50), {255, 255, 220, 255});
                string score_board = to_string(l_char.get_num_of_goals());
                string gtime = to_string(game_timer.get_time());

                score_board += ":";
                score_board += to_string(r_char.get_num_of_goals());

                render_text_center(m_renderer, gtime.c_str(), new SDL_Point{WIDTH / 2, HEIGHT - 50}, TTF_OpenFont("score_board.ttf", 60), {240, 220, 220, 255});
                render_text_center(m_renderer, score_board.c_str(), new SDL_Point{WIDTH / 2, 50}, TTF_OpenFont("score_board.ttf", 80), {240, 220, 220, 255});

                // Check for end
                if (game_timer.get_time() > 90 || l_char.get_num_of_goals() > 9 || r_char.get_num_of_goals() > 9)
                {
                    Game_State = STATE_END_MENU;
                }

                window_stuff(m_renderer, e);
            }
        }
        if (Game_State == STATE_PAUSE_MENU)
        {

            while (Game_State == STATE_PAUSE_MENU)
            {
                clear_window(m_renderer);
                window_stuff(m_renderer, e);
            }
        }
        if (Game_State == STATE_END_MENU)
        {

            while (Game_State == STATE_END_MENU)
            {
                clear_window(m_renderer);
                Game_State = STATE_QUIT;
                window_stuff(m_renderer, e);
            }
        }
        if (Game_State == STATE_QUIT)
        {
            Timer time;
            time.set_alarm(3);
            while (Game_State == STATE_QUIT)
            {
                clear_window(m_renderer);
                render_text_center(m_renderer, "Thank you!", new SDL_Point{WIDTH / 2, HEIGHT / 2}, TTF_OpenFont(FONT_ADDR, 70), {250, 255, 220, 255});
                if (time.check_alarm())
                {
                    break;
                }
                window_stuff(m_renderer , e);
            }
            if (time.check_alarm())
            {
                break;
            }
        }
    }

    // Finalize and free resources
    SDL_DestroyWindow(m_window);
    SDL_DestroyRenderer(m_renderer);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
