// Copyright (c) 2025, kiwec, All rights reserved.
#include "BottomBar.h"

#include "AnimationHandler.h"
#include "OsuConVars.h"
#include "Engine.h"
#include "i18n.h"
#include "VolNormalization.h"
#include "BatchDiffCalc.h"
#include "Mouse.h"
#include "OptionsOverlay.h"
#include "UIBackButton.h"
#include "Osu.h"
#include "MapExporter.h"
#include "ResourceManager.h"
#include "Skin.h"
#include "SkinImage.h"
#include "SongBrowser.h"
#include "UI.h"
#include "UserCard.h"
#include "Font.h"
#include "SyncMutex.h"
#include "Graphics.h"
#include "UIDraw.h"
#include "UITheme.h"
#include "Icons.h"
#include "UniString.h"

#include <atomic>
#include <cassert>

namespace BottomBar {
using namespace neomod::sbr;

namespace {

Sync::mutex export_progress_mtx;
std::atomic<float> export_progress{-1.f};
std::string export_entry;
std::string export_collection;

struct ButtonState {
    McRect rect{};
    AnimFloat alpha;  // hover alpha
};
std::array<ButtonState, BTN_MAX> btns{};
Button hovered_btn = BTN_NONE;

}  // namespace

void update_export_progress(float progress, std::string entry_being_processed, const std::string& collection) {
    export_progress.store(progress, std::memory_order_relaxed);
    Sync::scoped_lock lk(export_progress_mtx);
    export_collection = collection;
    if(progress > 0.f) export_entry = std::move(entry_being_processed);
}

void press_button(Button btn_index) {
    assert(btn_index < BTN_MAX);

    if(hovered_btn != btn_index) {
        btns[btn_index].alpha = 1.f;
        btns[btn_index].alpha.set(0.0f, 0.1f, anim::Linear, 0.05f);
    }

    switch(btn_index) {
        case MODE:
            return g_songbrowser->onSelectionMode();
        case MODS:
            return g_songbrowser->onSelectionMods();
        case RANDOM:
            return g_songbrowser->onSelectionRandom();
        case OPTIONS:
            return g_songbrowser->onSelectionOptions();
        default:
            std::unreachable();
            break;
    }
    std::unreachable();
}

f32 get_min_height() { return SongBrowser::getUIScale() * 101.f; }

namespace {
// the theme draws the four buttons itself unless it's classic or the skin brings its own selection-* images
bool redesigned_buttons() {
    const auto* skin = osu->getSkin();
    return !UITheme::classic() && skin->usesDefault(skin->i_sel_mode) && skin->usesDefault(skin->i_sel_mods) &&
           skin->usesDefault(skin->i_sel_random) && skin->usesDefault(skin->i_sel_options);
}
}  // namespace

void draw_redesigned_buttons();
void draw_skin_buttons();
void draw_background_tasks();

f32 get_height() {
    f32 max = 0.f;
    for(const auto& [rect, _] : btns) {
        if(rect.getHeight() > max) {
            max = rect.getHeight();
        }
    }

    return std::max(get_min_height(), max);
}

void tick() {
    const auto* skin = osu->getSkin();
    const vec2 screen = osu->getVirtScreenSize();
    bool is_widescreen = (screen.x / screen.y) > (4.f / 3.f);

    if(redesigned_buttons()) {
        // after the back button: four square-ish buttons, then the user card
        const f32 h = get_min_height();
        const f32 w = h * 1.08f;
        const f32 x0 = g_songbrowser->backButton->getSize().x + h * 0.18f;
        for(int i = 0; i < 4; i++) {
            btns[i].rect = McRect{x0 + (f32)i * w, screen.y - h, w, h};
        }
        osu->getUserButton()->setSize(SongBrowser::getUIScale(320.f), SongBrowser::getUIScale(75.f));
        osu->getUserButton()->setPos(btns[OPTIONS].rect.getX() + w + h * 0.35f,
                                     screen.y - h + (h - osu->getUserButton()->getSize().y) * 0.5f);
        osu->getUserButton()->tick();
        return;
    }

    const SkinImage& mode_img = skin->i_sel_mode_over;
    btns[MODE].rect.setSize(SongBrowser::getSkinDimensions(mode_img));
    btns[MODE].rect.setPos({Osu::getUIScale(is_widescreen ? 140.0f : 120.0f), screen.y - btns[MODE].rect.getHeight()});

    const SkinImage& mods_img = skin->i_sel_mods_over;
    btns[MODS].rect.setSize(SongBrowser::getSkinDimensions(mods_img));
    btns[MODS].rect.setPos(
        {btns[MODE].rect.getX() + SongBrowser::getUIScale(92.5f), screen.y - btns[MODS].rect.getHeight()});

    const SkinImage& random_img = skin->i_sel_random_over;
    btns[RANDOM].rect.setSize(SongBrowser::getSkinDimensions(random_img));
    btns[RANDOM].rect.setPos(
        {btns[MODS].rect.getX() + SongBrowser::getUIScale(77.5f), screen.y - btns[RANDOM].rect.getHeight()});

    const SkinImage& options_img = skin->i_sel_options_over;
    btns[OPTIONS].rect.setSize(SongBrowser::getSkinDimensions(options_img));
    btns[OPTIONS].rect.setPos(
        {btns[RANDOM].rect.getX() + SongBrowser::getUIScale(77.5f), screen.y - btns[OPTIONS].rect.getHeight()});

    osu->getUserButton()->setSize(SongBrowser::getUIScale(320.f), SongBrowser::getUIScale(75.f));
    osu->getUserButton()->setPos(btns[OPTIONS].rect.getX() + SongBrowser::getUIScale(160.f),
                                 osu->getVirtScreenHeight() - osu->getUserButton()->getSize().y);
    osu->getUserButton()->tick();
}

void updateInput(CBaseUIEventCtx& c) {
    static bool mouse_was_down = false;

    auto mousePos = mouse->getPos();
    bool clicked = !mouse_was_down && mouse->isLeftDown();
    mouse_was_down = mouse->isLeftDown();

    // the user card draws on top of the carousel at the bottom of the screen, so it must win the
    // click there; its bDrawsOnTop flag (set after construction) ranks it above the full-height
    // carousel scrollview that is visited later in the songbrowser walk
    osu->getUserButton()->updateInput(c);

    // TODO: BottomBar isn't a CBaseUIElement so click/hover detection needs BS manual workarounds
    bool overlay_has_mouse_inside = false;
    for(auto* overlay : {ui->getOptionsOverlayBase(), ui->getChatBase(), ui->getModSelectorBase(),
                         ui->getBeatmapInstallOverlayBase()}) {
        if(overlay->isVisible() && overlay->isMouseInside()) {
            overlay_has_mouse_inside = true;
            break;
        }
    }

    // Yes, the order looks whack. That's the correct order.
    Button new_hover = BTN_NONE;
    if(overlay_has_mouse_inside) {
        clicked = false;
    } else {
        if(btns[OPTIONS].rect.contains(mousePos)) {
            new_hover = OPTIONS;
        } else if(btns[MODE].rect.contains(mousePos)) {
            new_hover = MODE;
        } else if(btns[MODS].rect.contains(mousePos)) {
            new_hover = MODS;
        } else if(btns[RANDOM].rect.contains(mousePos)) {
            new_hover = RANDOM;
        } else {
            clicked = false;
        }
    }

    if(hovered_btn != new_hover) {
        if(hovered_btn != BTN_NONE) {
            btns[hovered_btn].alpha.set(0.0f, f32(btns[hovered_btn].alpha) * 0.1f, anim::Linear);
        }
        if(new_hover != BTN_NONE) {
            btns[new_hover].alpha.set(1.f, 0.1f, anim::Linear);
        }

        hovered_btn = new_hover;
    }

    if(clicked && c.propagate_clicks) {
        c.propagate_clicks = false;
        press_button(hovered_btn);
    }

    // TODO @kiwec: do hovers make sound?
}

void draw() {
    const auto* skin = osu->getSkin();
    const vec2 screen_size = osu->getVirtScreenSize();

    const f32 bar_height = get_min_height();
    if(!UITheme::classic() && skin->usesDefault(skin->i_songselect_bot)) {
        // the redesign: frosted glass with a thin glowing line along the top
        const auto& theme = UITheme::current();
        const f32 s = SongBrowser::getUIScale();
        const f32 top = screen_size.y - bar_height;
        UIDraw::glass({.rect = McRect{0.f, top, screen_size.x, bar_height}, .softness = 0.f}, theme.bar);
        const std::array<vec2, 2> line{vec2{0.f, top}, {screen_size.x, top}};
        UIDraw::glowLine(line, theme.line, 1.5f * s, Color(theme.lineGlow).setA(theme.lineGlow.Af() * 0.35f), 5.f * s);
    } else {
        g->pushTransform();
        {
            const Image* img = skin->i_songselect_bot;
            g->setColor(0xffffffff);
            g->scale((f32)screen_size.x / (f32)img->getWidth(), bar_height / (f32)img->getHeight());
            g->translate(0, screen_size.y - bar_height);
            g->drawImage(img, AnchorPoint::TOP_LEFT);
        }
        g->popTransform();
    }

    // don't double-draw the back button
    if(!ui->getOptionsOverlay()->isVisible()) {
        g_songbrowser->backButton->draw();
    }

    // Draw the user card under selection elements, which can cover it for fancy effects
    // (we don't match stable perfectly, but close enough)
    osu->getUserButton()->draw();

    if(redesigned_buttons()) {
        draw_redesigned_buttons();
    } else {
        draw_skin_buttons();
    }
    draw_background_tasks();
}

void draw_redesigned_buttons() {
    const auto& theme = UITheme::current();
    const f32 h = get_min_height();
    const f32 top = osu->getVirtScreenSize().y - h;
    McFont* icons = osu->getFontIcons();
    McFont* font = osu->getSubTitleFont();
    constexpr std::array<char32_t, 4> glyphs{Icons::DOT_CIRCLE_O, Icons::PLUS, Icons::RANDOM, Icons::BARS};
    constexpr std::array<const char*, 4> labels{"MODE", "MODS", "RANDOM", "OPTIONS"};
    constexpr std::array<const char*, 4> keys{"", "F1", "F2", "F3"};

    for(int i = 0; i < 4; i++) {
        const McRect& r = btns[i].rect;
        const f32 hover = std::clamp<f32>(btns[i].alpha, 0.f, 1.f);
        const f32 cx = r.getX() + r.getWidth() * 0.5f;

        if(hover > 0.f) {
            UIDraw::fill(
                UIDraw::Shape::rounded(McRect{r.getX() + h * 0.06f, top + h * 0.1f, r.getWidth() - h * 0.12f, h * 0.8f},
                                       h * 0.08f),
                Color(theme.chip).setA(theme.chip.Af() * hover * 1.6f));
        }

        // stable's coloured marker, hanging from the bar's top line
        const f32 mw = h * 0.42f, mh = std::max(2.f, h * 0.045f);
        const UIDraw::Shape marker{.rect = McRect{cx - mw * 0.5f, top, mw, mh}, .radii = {0.f, 0.f, mh, mh}};
        UIDraw::glow(marker, h * 0.08f, Color(theme.footMarks[i]).setA(0.55f + 0.45f * hover));
        UIDraw::fill(marker, theme.footMarks[i]);

        const std::string glyph = UniString::to_utf8(std::u32string(1, glyphs[i]));
        const f32 iconScale = (h * 0.27f) / icons->getHeight();
        g->setColor(UITheme::mix(theme.ink, theme.footMarks[i], hover * 0.6f));
        g->pushTransform();
        {
            g->scale(iconScale, iconScale);
            g->translate(cx - icons->getStringWidth(glyph) * iconScale * 0.5f, top + h * 0.47f - hover * h * 0.03f);
            g->drawString(icons, glyph);
        }
        g->popTransform();

        const std::string label = keys[i][0] ? fmt::format("{} {}", labels[i], keys[i]) : std::string{labels[i]};
        const f32 textScale = (h * 0.105f) / font->getHeight();
        g->setColor(theme.ink2);
        g->pushTransform();
        {
            g->scale(textScale, textScale);
            g->translate(cx - font->getStringWidth(label) * textScale * 0.5f, top + h * 0.78f);
            g->drawString(font, label);
        }
        g->popTransform();
    }
}

void draw_skin_buttons() {
    const auto* skin = osu->getSkin();
    const vec2 screen_size = osu->getVirtScreenSize();

    g->setColor(0xffffffff);

    // Careful, these buttons are often used as overlays
    // eg. selection-mode usually covers the whole screen, drawing topbar, bottom right osu cookie etc
    const std::array btn_imgs{std::pair{&btns[MODE], &skin->i_sel_mode},         //
                              std::pair{&btns[MODS], &skin->i_sel_mods},         //
                              std::pair{&btns[RANDOM], &skin->i_sel_random},     //
                              std::pair{&btns[OPTIONS], &skin->i_sel_options}};  //
    for(const auto& [btn, img] : btn_imgs) {
        if(img == nullptr) continue;

        const f32 scale = SongBrowser::getSkinScale(*img);
        img->drawRaw(vec2(btn->rect.getX(), screen_size.y), scale, AnchorPoint::BOTTOM_LEFT);
    }

    // Ok, and now for the hover images... which are drawn in a weird order, same as update_bottombar()
    const std::array btn_hoverimgs{std::pair{&btns[RANDOM], &skin->i_sel_random_over},     //
                                   std::pair{&btns[MODS], &skin->i_sel_mods_over},         //
                                   std::pair{&btns[MODE], &skin->i_sel_mode_over},         //
                                   std::pair{&btns[OPTIONS], &skin->i_sel_options_over}};  //
    for(const auto& [btn, img] : btn_hoverimgs) {
        if(img == nullptr) continue;

        g->setAlpha(btn->alpha);
        const f32 scale = SongBrowser::getSkinScale(*img);
        img->drawRaw(vec2(btn->rect.getX(), screen_size.y), scale, AnchorPoint::BOTTOM_LEFT);
    }

    // mode-osu-small (often used as overlay)
    const SkinImage& mos_img = skin->i_mode_osu_small;
    if(!mos_img.isMissingTexture()) {
        f32 mos_scale = SongBrowser::getSkinScale(mos_img);
        g->setBlendMode(DrawBlendMode::ADDITIVE);
        g->setColor(0xffffffff);
        mos_img.drawRaw(vec2(btns[MODE].rect.getX() + (btns[MODS].rect.getX() - btns[MODE].rect.getX()) / 2.f,
                             screen_size.y - SongBrowser::getUIScale(56.f)),
                        mos_scale, AnchorPoint::CENTER);
        g->setBlendMode(DrawBlendMode::ALPHA);
    }
}

void draw_background_tasks() {
    // background task busy notification
    g->setColor(UITheme::classic() ? Color(0xff333333) : UITheme::current().ink2);

    // XXX: move this to permanent toasts
    McFont* font = engine->getDefaultFont();
    i32 calcx = osu->getUserButton()->getPos().x + osu->getUserButton()->getSize().x + 20;
    i32 calcy = osu->getUserButton()->getPos().y + 15;
    if(BatchDiffCalc::get_maps_total() > 0) {
        std::string msg = tformat("Calculating stars ({}/{}) ...", BatchDiffCalc::get_maps_processed(),
                                  BatchDiffCalc::get_maps_total());
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    if(float progress = export_progress.load(std::memory_order_relaxed); progress > 0.f && progress < 1.f) {
        std::string export_entry_copy;
        std::string export_collection_copy;
        {
            Sync::scoped_lock lk(export_progress_mtx);
            export_entry_copy = export_entry;
            export_collection_copy = export_collection;
        }
        std::string msg1 =
            tformat("Exporting {} {:.2f}%", !export_collection_copy.empty() ? export_collection_copy : _("mapset"),
                    progress * 100.f);
        std::string msg2 = tformat(" {:s}", export_entry_copy);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg1);
        g->translate(0, font->getHeight() + 10);
        g->drawString(font, msg2);
        g->popTransform();
        calcy += font->getHeight() + 20;
    }
    if(cv::normalize_loudness.getBool() && VolNormalization::get_total() > 0 &&
       VolNormalization::get_computed() < VolNormalization::get_total()) {
        std::string msg =
            tformat("Computing loudness ({}/{}) ...", VolNormalization::get_computed(), VolNormalization::get_total());
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
    const auto calc_total = BatchDiffCalc::get_scores_total();
    const auto calc_computed = BatchDiffCalc::get_scores_processed();
    if(calc_total > 0 && calc_computed < calc_total) {
        std::string msg = tformat("Converting scores ({}/{}) ...", calc_computed, calc_total);
        g->pushTransform();
        g->translate(calcx, calcy);
        g->drawString(font, msg);
        g->popTransform();
        calcy += font->getHeight() + 10;
    }
}

// TODO @kiwec: default icon for mode-osu-small
}  // namespace BottomBar
