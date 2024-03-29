/**
 * CosmoVM an emulator and assembler for an imaginary cpu
 * Copyright (C) 2022 JeSuis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <cstring>

#include <cosmovm/display.hpp>

using namespace cosmovm;

display::display(std::shared_ptr<bus>& bus, const std::string& window_title)
:
m_bus(bus),
m_video_mem_buf(m_bus->get_memory()->get_buf().begin() + VIDEO_START_ADDR),
m_mode(VIDEO_MODES::TEXT)
{
    SDL_CreateWindowAndRenderer(
        WINDOW_W,
        WINDOW_H,
        0,
        &m_window,
        &m_renderer);
    SDL_SetWindowTitle(m_window, window_title.c_str());
    m_color = {0xDF, 0xDF, 0xDF, 0};

    if((m_font = TTF_OpenFont(FONT_PATH.c_str(), 8)) == NULL)
        throw std::invalid_argument(std::format("[DISPLAY] Couldn't find {}", FONT_PATH));

    // Arbitrary port
    m_bus->bind_port(0x44, std::bind(&display::change_mode, this, std::placeholders::_1));
}

display::~display()
{
    TTF_CloseFont(m_font);
    SDL_DestroyRenderer(m_renderer);
    SDL_DestroyWindow(m_window);
}

void display::run()
{
    switch (m_mode)
    {
        case VIDEO_MODES::TEXT:
            render_text_mode();
            break;
        case VIDEO_MODES::GRAPHIC:
            render_graphic_mode();
            break;
        default:
            break;
    }
}

bool display::window_is_open()
{
    return !m_quit;
}

u16 display::change_mode(u16 mode)
{
    switch (m_mode)
    {
        case VIDEO_MODES::TEXT:
            m_mode = static_cast<VIDEO_MODES>(mode);
            break;
        case VIDEO_MODES::GRAPHIC:
            m_mode = static_cast<VIDEO_MODES>(mode);
            break;
        default:
            throw std::invalid_argument(std::format("[DISPLAY] Unknown mode {}", mode));
            break;
    }
    // Return dummy
    return PORT_DUMMY_VALUE;
}

void display::render_text_mode()
{
    SDL_Surface* surface;
    SDL_Texture* texture;
    SDL_Rect rect;
    SDL_Event event;

    if (*m_video_mem_buf != 0)
    {
        surface =
            TTF_RenderUTF8_Solid_Wrapped(
                m_font,
                reinterpret_cast<const char*>(&(*m_video_mem_buf)),
                m_color,
                WINDOW_W);
        texture = SDL_CreateTextureFromSurface(m_renderer, surface);
        rect = {.x = 0, .y = 0, .w = surface->w, .h = static_cast<int>(surface->h * 2.125F)};
    }

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            m_quit = 1;
    }
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);
    if (*m_video_mem_buf != 0)
        SDL_RenderCopy(m_renderer, texture, NULL, &rect);
    SDL_RenderPresent(m_renderer);

    if (*m_video_mem_buf != 0)
    {
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
}

void display::render_graphic_mode()
{
    SDL_Surface* surface;
    SDL_Texture* texture;
    SDL_Rect rect;
    SDL_Event event;
    surface =
        SDL_CreateRGBSurfaceFrom(
            (void*)m_video_mem_buf,
            GFX_MODE_W,
            GFX_MODE_H,
            GFX_BIT_DEPTH,
            GFX_MODE_W,
            0b11100000,
            0b00011100,
            0b00000011,
            0b0);
    texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    rect = {.x = 0, .y = 0, .w = surface->w * (WINDOW_W / GFX_MODE_W), .h = surface->h * (WINDOW_H / GFX_MODE_H)};

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            m_quit = 1;
    }
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 0);
    SDL_RenderClear(m_renderer);
    SDL_RenderCopy(m_renderer, texture, NULL, &rect);
    SDL_RenderPresent(m_renderer);

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}