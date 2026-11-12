#include <SDL.h>            

#include "simple_json.h"
#include "simple_logger.h"

#include "gfc_input.h"
#include "gfc_config_def.h"
#include "gfc_vector.h"
#include "gfc_matrix.h"
#include "gfc_audio.h"
#include "gfc_string.h"
#include "gfc_actions.h"

#include "gf2d_sprite.h"
#include "gf2d_font.h"
#include "gf2d_actor.h"
#include "gf2d_mouse.h"

#include "gf3d_vgraphics.h"
#include "gf3d_pipeline.h"
#include "gf3d_swapchain.h"
#include "gf3d_mesh.h"

#include "model.h"
#include "entity.h"
#include "pokemon.h"
#include "player.h"
#include "level.h"
#include "item.h"
#include "draw_3d.h"
#include "turn.h"
#include "battle.h"

extern int __DEBUG;

static int _done = 0;
static Uint32 frame_delay = 16;
static float fps = 0;

void parse_arguments(int argc,char *argv[]);
void game_frame_delay();

void exitGame()
{
    _done = 1;
}


int main(int argc,char *argv[])
{
    //local variables
    Level* level;
    GFC_TextLine text;
    Entity* player;
    Uint8 i;
    Sprite* textBG;
    GFC_Color textBGColor, textHPColor;
    char arrows[4];
    //initialization    
    parse_arguments(argc,argv);
    init_logger("gf3d.log",0);
    slog("gf3d begin");
    //gfc init
    gfc_input_init("config/input.cfg");
    gfc_config_def_init();
    gfc_action_init(1024);
    //gf3d init
    gf3d_vgraphics_init("config/setup.cfg");
    draw_3d_init();
    gfc_sound_init_config("config/audio.cfg");
    gf2d_font_init("config/font.cfg");
    gf2d_actor_init(1000);
    gfc_config_def_load("defs/item.json");
    gfc_config_def_load("defs/pokemon.json");
    gfc_config_def_load("defs/move.json");
    gfc_config_def_load("defs/type.json");
    gfc_config_def_load("defs/stat.json");
    entity_init(1000);
    turn_init(5, 20, 1000);
    battle_init();
    
    //game init
    srand(SDL_GetTicks());
    slog_sync();
    gf2d_mouse_load("actors/mouse.actor");
    level = level_load("levels/testlevel.json");
    player_spawn(gfc_vector3d(36, 28, 0), "Mudkip");
    pokemon_spawn(gfc_vector3d(37, 28, 0), "Treecko", 0);
    pokemon_spawn(gfc_vector3d(35, 28, 0), "Torchic", 0);
    pokemon_spawn(gfc_vector3d(36, 27, 0), "Pikachu (M)", 0);
    pokemon_spawn(gfc_vector3d(37, 27, 0), "Skitty", 0);
    pokemon_enemy_spawn_random();
    pokemon_enemy_spawn_random();
    pokemon_enemy_spawn_random();
    textBG = gf2d_sprite_load("images/text_bg.png", 200, 160, 1);
    textBGColor = gfc_color8(0, 0, 64, 255);
    arrows[0] = '^';
    arrows[1] = '>';
    arrows[2] = 'v';
    arrows[3] = '<';

    // main game loop    
    while(!_done)
    {
        gfc_input_update();
        gf2d_mouse_update();
        gf2d_font_update();

        // camera updates

        // thinks
        entity_system_think();

        // entity updates
        entity_system_update();

        // start draw
        gf3d_vgraphics_render_start();
        player = player_get_the();

        // 3D draws
        level_draw(level);
        entity_system_draw();

        // 2D draws
        if (player)
        {
            snprintf(text, sizeof(GFC_TextLine), "%i/%i HP", player->hp, player->maxHP);
            if ((float)(player->hp) / player->maxHP <= 0.25) textHPColor = GFC_COLOR_RED;
            else if ((float)(player->hp) / player->maxHP <= 0.5) textHPColor = GFC_COLOR_YELLOW;
            else textHPColor = GFC_COLOR_DARKGREEN;
            gf2d_font_draw_line_tag(text, FT_H1, textHPColor, gfc_vector2d(10, 10));
            if (gfc_input_command_down("show_moves") && !turn_anybody_moving())
            {
                gf2d_sprite_draw(
                    textBG,
                    gfc_vector2d(540, 280),
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    &textBGColor,
                    NULL,
                    0
                );
                for (i = 0; i < 4; i++)
                {
                    snprintf(
                        text,
                        sizeof(GFC_TextLine),
                        "%c %s %i/%i PP",
                        arrows[i],
                        player->moves[i]->name,
                        player->currentPP[i],
                        player->moves[i]->maxPP
                    );
                    gf2d_font_draw_line_tag(
                        text,
                        FT_H6,
                        GFC_COLOR_WHITE,
                        gfc_vector2d(550, 292 + 40 * i)
                    );
                }
            }
        }
        gf2d_mouse_draw();

        gf3d_vgraphics_render_end();
        if (!player || gfc_input_command_down("exit"))_done = 1; // exit condition
        game_frame_delay();
    }    
    vkDeviceWaitIdle(gf3d_vgraphics_get_default_logical_device());    
    //cleanup
    slog("gf3d program end");
    exit(0);
    slog_sync();
    return 0;
}

void parse_arguments(int argc,char *argv[])
{
    int a;

    for (a = 1; a < argc;a++)
    {
        if (strcmp(argv[a],"--debug") == 0)
        {
            __DEBUG = 1;
        }
    }    
}

void game_frame_delay()
{
    Uint32 diff;
    static Uint32 now;
    static Uint32 then;
    then = now;
    slog_sync();// make sure logs get written when we have time to write it
    now = SDL_GetTicks();
    diff = (now - then);
    if (diff < frame_delay)
    {
        SDL_Delay(frame_delay - diff);
    }
    fps = 1000.0/MAX(SDL_GetTicks() - then,0.001);
        //slog("fps: %f",fps);
}
/*eol@eof*/
