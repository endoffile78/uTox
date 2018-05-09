#include "ui.h"

#include "flist.h"
#include "friend.h"
#include "groups.h"
#include "inline_video.h"
#include "macros.h"
#include "messages.h"
#include "screen_grab.h"
#include "self.h"
#include "settings.h"
#include "tox.h"
#include "updater.h"

#include "av/utox_av.h"
#include "av/video.h"

#include "layout/background.h"
#include "layout/create.h"
#include "layout/friend.h"
#include "layout/group.h"
#include "layout/notify.h"
#include "layout/settings.h"
#include "layout/sidebar.h"

#include "native/image.h"
#include "native/keyboard.h"
#include "native/ui.h"

#include "ui/button.h"
#include "ui/contextmenu.h"
#include "ui/draw.h"
#include "ui/dropdown.h"
#include "ui/edit.h"
#include "ui/panel.h"
#include "ui/scrollable.h"
#include "ui/switch.h"
#include "ui/text.h"
#include "ui/tooltip.h"

#include <stdlib.h>
#include <string.h>
#include <ui.h>

#define UI_SIDEBAR_INDEX 0
#define UI_MAIN_INDEX 0

char *entry_password_text = NULL;

//These need to be global for ui_show_page and ui_hide_page
static uiBox *sidebar_vbox;
static uiBox *main_vbox;

static PAGE_TYPE current_page;

bool ui_initialized = false;

static UTOX_I18N_STR themedrops[] = {
    STR_THEME_DEFAULT,
    STR_THEME_LIGHT,
    STR_THEME_DARK,
    STR_THEME_HIGHCONTRAST,
    STR_THEME_CUSTOM,
    STR_THEME_ZENBURN,
    STR_THEME_SOLARIZED_LIGHT,
    STR_THEME_SOLARIZED_DARK,
};

static UTOX_I18N_STR dpidrops[] = {
    STR_DPI_TINY, STR_DPI_060,   STR_DPI_070, STR_DPI_080, STR_DPI_090, STR_DPI_NORMAL, STR_DPI_110,
    STR_DPI_120,  STR_DPI_130,   STR_DPI_140, STR_DPI_BIG, STR_DPI_160, STR_DPI_170,    STR_DPI_180,
    STR_DPI_190,  STR_DPI_LARGE, STR_DPI_210, STR_DPI_220, STR_DPI_230, STR_DPI_240,    STR_DPI_HUGE,
};

/* These remain for legacy reasons, PANEL_MAIN calls these by default when not given it's own function to call */
static void background_draw(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height)) {
    return;
}

static bool background_mmove(PANEL *UNUSED(p), int UNUSED(x), int UNUSED(y), int UNUSED(width), int UNUSED(height),
                      int UNUSED(mx), int UNUSED(my), int UNUSED(dx), int UNUSED(dy)) {
    return false;
}

static bool background_mdown(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mright(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mwheel(PANEL *UNUSED(p), int UNUSED(height), double UNUSED(d), bool UNUSED(smooth)) {
    return false;
}

static bool background_mup(PANEL *UNUSED(p)) {
    return false;
}

static bool background_mleave(PANEL *UNUSED(p)) {
    return false;
}

/***** MAYBE_I18NAL_STRING helpers start *****/

void maybe_i18nal_string_set_plain(MAYBE_I18NAL_STRING *mis, char *str, uint16_t length) {
    mis->i18nal       = UI_STRING_ID_INVALID;
    mis->plain.length = length;
    mis->plain.str    = str;
}

void maybe_i18nal_string_set_i18nal(MAYBE_I18NAL_STRING *mis, UTOX_I18N_STR string_id) {
    mis->plain.str    = NULL;
    mis->plain.length = 0;
    mis->i18nal       = string_id;
}

STRING *maybe_i18nal_string_get(MAYBE_I18NAL_STRING *mis) {
    if (mis->plain.str) {
        return &mis->plain;
    }

    return SPTRFORLANG(settings.language, mis->i18nal);
}

bool maybe_i18nal_string_is_valid(MAYBE_I18NAL_STRING *mis) {
    return (mis->plain.str || ((UI_STRING_ID_INVALID != mis->i18nal) && (mis->i18nal < NUM_STRS)));
}

/***********************************************************************
 *                                                                     *
 * Panel layout size set functions.                                    *
 *                                                                     *
 **********************************************************************/
static void sidepanel_USERBADGE(void) {
    // Converting DEFINES to magic becaues this will be moved to layout/
    // and will then get a different format/selection
    CREATE_BUTTON(avatar, SIDEBAR_AVATAR_LEFT, SIDEBAR_AVATAR_TOP, 40, 40);
    CREATE_BUTTON(name, SIDEBAR_NAME_LEFT, SIDEBAR_NAME_TOP, SIDEBAR_NAME_WIDTH, SIDEBAR_NAME_HEIGHT - 2);
    CREATE_BUTTON(status_msg, SIDEBAR_STATUSMSG_LEFT, SIDEBAR_STATUSMSG_TOP, SIDEBAR_STATUSMSG_WIDTH, SIDEBAR_STATUSMSG_HEIGHT - 2);
    CREATE_BUTTON(usr_state, 200, 10, 25, 45);
}

static void sidepanel_FLIST(void) {
    scrollbar_flist.panel.y      = 0;
    // scrollbar_flist.panel.width  = 230; // TODO remove?
    scrollbar_flist.panel.height = -1;

    panel_flist.x      = 0;
    panel_flist.y      = 70;
    panel_flist.width  = 230; // TODO remove?
    panel_flist.height = ROSTER_BOTTOM;


    CREATE_BUTTON(filter_friends, SIDEBAR_FILTER_FRIENDS_LEFT, SIDEBAR_FILTER_FRIENDS_TOP, SIDEBAR_FILTER_FRIENDS_WIDTH,
                  SIDEBAR_FILTER_FRIENDS_HEIGHT);
    CREATE_EDIT(search, SIDEBAR_SEARCH_LEFT, SIDEBAR_SEARCH_TOP, SIDEBAR_SEARCH_WIDTH, SIDEBAR_SEARCH_HEIGHT);

    CREATE_BUTTON(settings, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    CREATE_BUTTON(add_new_contact, SIDEBAR_BUTTON_LEFT, ROSTER_BOTTOM, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_HEIGHT);
    button_add_new_contact.panel.disabled = true;
}

void ui_set_scale(uint8_t scale) {
    if (scale >= 5 && scale <= 25) {
        ui_scale = scale;
    } else if (scale != 0) {
        return ui_set_scale(10);
    }
}

void ui_rescale(uint8_t scale) {
    ui_set_scale(scale);

    flist_re_scale();
    setscale_fonts();
    setfont(FONT_SELF_NAME);

    /* DEFAULT positions */

    panel_main.y = 0;

    scrollbar_settings.panel.y        = 32;  /* TODO magic numbers are bad */
    scrollbar_settings.content_height = 300; /* TODO magic numbers are bad */

    panel_settings_master.y        = 0;
    panel_settings_devices.y       = 32;
    panel_settings_adv.y           = 32;

    scrollbar_friend.panel.y      = MAIN_TOP;
    scrollbar_friend.panel.height = CHAT_BOX_TOP;
    messages_friend.y             = MAIN_TOP;
    messages_friend.height        = CHAT_BOX_TOP - 10;
    messages_friend.width         = -SCROLL_WIDTH;

    scrollbar_group.panel.y      = MAIN_TOP;
    scrollbar_group.panel.height = CHAT_BOX_TOP;
    messages_group.y             = MAIN_TOP;
    messages_group.height        = CHAT_BOX_TOP;
    messages_group.width         = -SCROLL_WIDTH;

    setfont(FONT_SELF_NAME);

    sidepanel_USERBADGE();
    sidepanel_FLIST();

    // FIXME for testing, remove
    CREATE_BUTTON(notify_create, 2, 2, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(notify_one, 0, -50, 40, 50);
    CREATE_BUTTON(notify_two, 200, -50, 40, 50);
    CREATE_BUTTON(notify_three, -40, -50, 40, 50);

    CREATE_BUTTON(move_notify, -40, -40, 40, 40);


    /* Setting pages */
    uint32_t settings_x = 4;
    CREATE_BUTTON(settings_sub_profile,         settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(PROFILE_BUTTON));

#ifdef ENABLE_MULTIDEVICE
    CREATE_BUTTON(settings_sub_devices,         settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(DEVICES_BUTTON));
#endif

    CREATE_BUTTON(settings_sub_ui,              settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(USER_INTERFACE_BUTTON));

    CREATE_BUTTON(settings_sub_av,              settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(AUDIO_VIDEO_BUTTON));

    CREATE_BUTTON(settings_sub_notifications,   settings_x, 0, 12, 28);
    settings_x += 20 + UN_SCALE(UTOX_STR_WIDTH(NOTIFICATIONS_BUTTON));

    CREATE_BUTTON(settings_sub_adv,             settings_x, 0, 12, 28);


    /* Devices */
    CREATE_BUTTON(add_new_device_to_self, -10 - BM_SBUTTON_WIDTH, 28, BM_SBUTTON_WIDTH, BM_SBUTTON_HEIGHT);

    CREATE_EDIT(add_new_device_to_self, 10, 27, 0 - UTOX_STR_WIDTH(ADD) - BM_SBUTTON_WIDTH, 24);


    /* Friend Add Page */
    CREATE_BUTTON(send_friend_request, -10 - _BM_SBUTTON_WIDTH, MAIN_TOP + 168, _BM_SBUTTON_WIDTH,
                  _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(accept_friend, -60, -80, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    /* Friend Settings Page */
    CREATE_BUTTON(export_chatlog, 10, 208, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    CREATE_EDIT(friend_pubkey,          10, 88, -10, 24);
    CREATE_EDIT(friend_alias,           10, 138, -10, 24);

    CREATE_SWITCH(friend_autoaccept_ft, 10, 168, _BM_SWITCH_WIDTH, _BM_SWITCH_HEIGHT);

    /* Group Settings */
    CREATE_EDIT(group_topic, 10, 95, -10, 24);

    /* Friend / Group Page  */
    CREATE_BUTTON(call_decline, -186, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_audio,   -124, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(call_video,    -62, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);
    CREATE_BUTTON(group_audio,   -62, 10, _BM_LBUTTON_WIDTH, _BM_LBUTTON_HEIGHT);

    CREATE_BUTTON(send_file,         6, -46, _BM_CHAT_BUTTON_WIDTH, _BM_CHAT_BUTTON_HEIGHT);
    CREATE_BUTTON(send_screenshot,   8 + _BM_CHAT_BUTTON_WIDTH, -46, _BM_CHAT_BUTTON_WIDTH, _BM_CHAT_BUTTON_HEIGHT);

    CREATE_BUTTON(chat_send_friend, -6 - _BM_CHAT_SEND_WIDTH, -46, _BM_CHAT_SEND_WIDTH, _BM_CHAT_SEND_HEIGHT);
    CREATE_BUTTON(chat_send_group,  -6 - _BM_CHAT_SEND_WIDTH, -46, _BM_CHAT_SEND_WIDTH, _BM_CHAT_SEND_HEIGHT);

    setfont(FONT_TEXT);

    // Add friend panel
    CREATE_EDIT(add_new_friend_id, 10, 28 + MAIN_TOP, -10, 24);
    CREATE_EDIT(add_new_friend_msg, 10, 76 + MAIN_TOP, -10, 84);

    /* Message entry box for friends and groups */
    CREATE_EDIT(chat_msg_friend, 10 + _BM_CHAT_BUTTON_WIDTH * 2, /* Make space for the left button  */
                -46, -64, 40); /* text is 8 high. 8 * 2.5 = 20. */

    CREATE_EDIT(chat_msg_group, 6, -46, -10 - BM_CHAT_SEND_WIDTH, 40);

    /* Confirm deletion */
    CREATE_BUTTON(confirm_deletion, 10, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);
    CREATE_BUTTON(deny_deletion,    110, MAIN_TOP + 40, _BM_SBUTTON_WIDTH, _BM_SBUTTON_HEIGHT);

    setscale();
}

/* Use the preprocessor to build function prototypes for all user interactions
 * These are functions that are (must be) defined elsewehere. The preprocessor in this case creates the prototypes that
 * will then be used by panel_draw_core to call the correct function
*/
#define MAKE_FUNC(ret, x, ...)                                                                                          \
    static ret (*x##func[])(void *p, ##__VA_ARGS__) = {                                                                 \
        (void *)background_##x, (void *)messages_##x, (void *)inline_video_##x, (void *)flist_##x,  (void *)button_##x, \
        (void *)switch_##x,     (void *)dropdown_##x, (void *)edit_##x,         (void *)scroll_##x,                     \
    };

MAKE_FUNC(void, draw, int x, int y, int width, int height);
MAKE_FUNC(bool, mmove, int x, int y, int width, int height, int mx, int my, int dx, int dy);
MAKE_FUNC(bool, mdown);
MAKE_FUNC(bool, mright);
MAKE_FUNC(bool, mwheel, int height, double d);
MAKE_FUNC(bool, mup);
MAKE_FUNC(bool, mleave);

#undef MAKE_FUNC

/* Use the preprocessor to add code to adjust the x,y cords for panels or sub panels.
 * If neg value place x/y from the right/bottom of panel.
 *
 * change the relative
 *
 * if w/h <0 use parent panel width (maybe?)    */
#define FIX_XY_CORDS_FOR_SUBPANELS()                                       \
    {                                                                      \
        int relx = (p->x < 0) ? width + SCALE(p->x) : SCALE(p->x);                       \
        int rely = (p->y < 0) ? height + SCALE(p->y) : SCALE(p->y);                      \
        x += relx;                                                         \
        y += rely;                                                         \
        width  = (p->width <= 0) ? width + SCALE(p->width) - relx : SCALE(p->width);     \
        height = (p->height <= 0) ? height + SCALE(p->height) - rely : SCALE(p->height); \
    }

static void panel_update(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    switch (p->type) {
        case PANEL_NONE: {
            if (p == &panel_settings_devices) {
                #ifdef ENABLE_MULTIDEVICE
                devices_update_ui();
                #endif
            }
            break;
        }

        case PANEL_MESSAGES: {
            if (p->object) {
                MESSAGES *m = p->object;
                m->width    = width;
                messages_updateheight(m, width);
            }
            break;
        }

        default: {
            break;
        }
    }

    PANEL **pp = p->child;
    if (pp) {
        if (p->update) {
            p->update(width, height, ui_scale);
        }

        PANEL *subp;
        while ((subp = *pp++)) {
            panel_update(subp, x, y, width, height);
        }
    }
}

void draw_avatar_image(NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t targetwidth,
                       uint32_t targetheight)
{
    /* get smallest of width or height */
    const double scale = (width > height) ? (double)targetheight / height : (double)targetwidth / width;

    image_set_scale(image, scale);
    image_set_filter(image, FILTER_BILINEAR);

    /* set position to show the middle of the image in the center  */
    const int xpos = (int)((double)width * scale / 2 - (double)targetwidth / 2);
    const int ypos = (int)((double)height * scale / 2 - (double)targetheight / 2);

    draw_image(image, x, y, targetwidth, targetheight, xpos, ypos);

    image_set_scale(image, 1.0);
    image_set_filter(image, FILTER_NEAREST);
}

void ui_size(int width, int height) {
    panel_update(&panel_root, 0, 0, width, height);
    tooltip_reset();

    panel_side_bar.disabled = false;
    panel_main.x = panel_flist.width;

    if (settings.magic_flist_enabled) {
        if (width <= panel_flist.width * 2 || height > width) {
            panel_side_bar.disabled = true;
            panel_main.x = 0;
        }
    }
}

void ui_mouseleave(void) {
    panel_mleave(&panel_root);
    tooltip_reset();
    redraw();
}

static void panel_draw_core(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    if (p->content_scroll) {
        pushclip(x, y, width, height);
        y -= scroll_gety(p->content_scroll, height);
    }

    if (p->type) {
        drawfunc[p->type - 1](p, x, y, width, height);
    } else {
        if (p->drawfunc) {
            p->drawfunc(x, y, width, height);
        }
    }

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                panel_draw_core(subp, x, y, width, height);
            }
        }
    }

    if (p->content_scroll) {
        popclip();
    }
}

void panel_draw(PANEL *p, int x, int y, int width, int height) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    panel_draw_core(p, x, y, width, height);

    // popclip();

    dropdown_drawactive();
    contextmenu_draw();
    tooltip_draw();

    enddraw(x, y, width, height);
}

bool panel_mmove(PANEL *p, int x, int y, int width, int height, int mx, int my, int dx, int dy) {
    if (p == &panel_root) {
        mouse.x = mx;
        mouse.y = my;
    }

    mx -= (p->x < 0) ? width + SCALE(p->x) : SCALE(p->x);
    my -= (p->y < 0) ? height + SCALE(p->y) : SCALE(p->y);
    FIX_XY_CORDS_FOR_SUBPANELS();

    int mmy = my;

    if (p->content_scroll) {
        const int scroll_y = scroll_gety(p->content_scroll, height);
        if (my < 0) {
            mmy = -1;
        } else if (my >= height) {
            mmy = 1024 * 1024 * 1024; // large value
        } else {
            mmy = my + scroll_y;
        }
        y -= scroll_y;
        my += scroll_y;
    }

    bool draw = p->type ? mmovefunc[p->type - 1](p, x, y, width, height, mx, mmy, dx, dy) : false;
    // Has to be called before children mmove
    if (p == &panel_root) {
        draw |= tooltip_mmove();
    }
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mmove(subp, x, y, width, height, mx, my, dx, dy);
            }
        }
    }

    if (p == &panel_root) {
        draw |= contextmenu_mmove(mx, my, dx, dy);
        if (draw) {
            redraw();
        }
    }

    return draw;
}

static bool panel_mdown_sub(PANEL *p) {
    if (p->type && mdownfunc[p->type - 1](p)) {
        return true;
    }

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                if (panel_mdown_sub(subp)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void panel_mdown(PANEL *p) {
    if (contextmenu_mdown() || tooltip_mdown()) {
        redraw();
        return;
    }

    bool draw = edit_active();

    PANEL **pp = p->child;

    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mdown_sub(subp);
            }
        }
    }

    if (draw) {
        redraw();
    }
}

bool panel_dclick(PANEL *p, bool triclick) {
    bool draw = false;
    if (p->type == PANEL_EDIT) {
        draw = edit_dclick((EDIT *)p, triclick);
    } else if (p->type == PANEL_MESSAGES) {
        draw = messages_dclick(p, triclick);
    }

    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw = panel_dclick(subp, triclick);
                if (draw) {
                    break;
                }
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

bool panel_mright(PANEL *p) {
    bool draw = p->type ? mrightfunc[p->type - 1](p) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mright(subp);
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

bool panel_mwheel(PANEL *p, int x, int y, int width, int height, double d, bool smooth) {
    FIX_XY_CORDS_FOR_SUBPANELS();

    bool draw = p->type ? mwheelfunc[p->type - 1](p, height, d) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mwheel(subp, x, y, width, height, d, smooth);
            }
        }
    }

    if (draw && p == &panel_root) {
        redraw();
    }

    return draw;
}

 bool panel_mup(PANEL *p) {
    if (p == &panel_root && contextmenu_mup()) {
        tooltip_mup();
        redraw();
        return true;
    }

    bool draw = p->type ? mupfunc[p->type - 1](p) : false;
    PANEL **pp = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mup(subp);
            }
        }
    }

    if (p == &panel_root) {
        tooltip_mup();
        if (draw) {
            redraw();
        }
    }

    return draw;
}

bool panel_mleave(PANEL *p) {
    bool    draw = p->type ? mleavefunc[p->type - 1](p) : false;
    PANEL **pp   = p->child;
    if (pp) {
        PANEL *subp;
        while ((subp = *pp++)) {
            if (!subp->disabled) {
                draw |= panel_mleave(subp);
            }
        }
    }

    if (p == &panel_root) {
        draw |= contextmenu_mleave();
        if (draw) {
            redraw();
        }
    }

    return draw;
}

static int ui_on_quit(uiWindow *win, void *UNUSED(data)){
    uiControlDestroy(uiControl(win));
    uiQuit();
    return 0;
}

static int ui_should_quit(void *data){
    uiWindow *window = (uiWindow *)data;
    uiControlDestroy(uiControl(window));
    return 1;
}

static void btn_copy_clicked(uiButton *UNUSED(button), void *UNUSED(data)){

}

static void btn_show_qr_clicked(uiButton *UNUSED(button), void *UNUSED(data)){

}

static void cbo_language_on_select(uiCombobox *UNUSED(combox), void *UNUSED(data)){

}

static uiControl *settings_profile_page(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(NAME))), 0);
    uiEntry *entry_name = uiNewEntry();
    uiEntrySetText(entry_name, self.name);
    uiBoxAppend(vbox, uiControl(entry_name), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(STATUS))), 0);
    uiEntry *entry_status = uiNewEntry();
    uiEntrySetText(entry_status, self.statusmsg);
    uiBoxAppend(vbox, uiControl(entry_status), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(TOXID))), 0);
    uiEntry *entry_id = uiNewEntry();
    uiEntrySetReadOnly(entry_id, 1);
    uiEntrySetText(entry_id, self.id_str);
    uiBoxAppend(vbox, uiControl(entry_id), 0);

    uiBoxAppend(vbox, uiControl(hbox), 0);

    //TODO: fix copy and show qrs buttons position
    uiButton *btn_copy = uiNewButton(S(COPY_TOX_ID));
    uiButtonOnClicked(btn_copy, btn_copy_clicked, NULL);
    uiBoxAppend(hbox, uiControl(btn_copy), 0);

    uiButton *btn_show_qr = uiNewButton(S(SHOW_QR));
    uiButtonOnClicked(btn_show_qr, btn_show_qr_clicked, NULL);
    uiBoxAppend(hbox, uiControl(btn_show_qr), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(LANGUAGE))), 0);
    uiCombobox *cbo_language = uiNewCombobox();
    uiComboboxOnSelected(cbo_language, cbo_language_on_select, NULL);
    uiBoxAppend(vbox, uiControl(cbo_language), 0);

    return uiControl(vbox);
}

static void cbo_theme_on_select(uiCombobox *combobox, void *UNUSED(data)){
    settings.theme = uiComboboxSelected(combobox);
}

static void cbo_dpi_on_select(uiCombobox *UNUSED(combobox), void *UNUSED(data)){
    //TODO: implement this
}

static void chk_history_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.logging_enabled = uiCheckboxChecked(checkbox);
}

static void chk_close_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.close_to_tray = uiCheckboxChecked(checkbox);
}

static void chk_start_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.start_in_tray = uiCheckboxChecked(checkbox);
}

static void chk_startup_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.start_with_system = uiCheckboxChecked(checkbox);
}

static void chk_mini_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.use_mini_flist = uiCheckboxChecked(checkbox);
}

static uiControl *settings_interface_page(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiBoxAppend(vbox, uiControl(hbox), 0);

    //TODO: theme and dpi dropdown, currently libui doesn't have support for either
    uiCombobox *cbo_theme = uiNewCombobox();
    for (size_t i = 0; i < COUNTOF(themedrops); i++){
        char *str = SPTRFORLANG(settings.language, themedrops[i])->str;
        uiComboboxAppend(cbo_theme, str);
    }
    uiComboboxOnSelected(cbo_theme, cbo_theme_on_select, NULL);
    uiBoxAppend(hbox, uiControl(cbo_theme), 0);

    uiCombobox *cbo_dpi = uiNewCombobox();
    for (size_t i = 0; i < COUNTOF(dpidrops); i++) {
        char *str = SPTRFORLANG(settings.language, dpidrops[i])->str;
        uiComboboxAppend(cbo_dpi, str);
    }
    uiComboboxOnSelected(cbo_dpi, cbo_dpi_on_select, NULL);
    uiBoxAppend(hbox, uiControl(cbo_dpi), 0);

    uiCheckbox *chk_history = uiNewCheckbox(S(SAVE_CHAT_HISTORY));
    uiCheckboxSetChecked(chk_history, settings.logging_enabled);
    uiCheckboxOnToggled(chk_history, chk_history_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_history), 0);

    uiCheckbox *chk_close = uiNewCheckbox(S(CLOSE_TO_TRAY));
    uiCheckboxSetChecked(chk_close, settings.close_to_tray);
    uiCheckboxOnToggled(chk_close, chk_close_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_close), 0);

    uiCheckbox *chk_start = uiNewCheckbox(S(START_IN_TRAY));
    uiCheckboxSetChecked(chk_start, settings.start_in_tray);
    uiCheckboxOnToggled(chk_start, chk_start_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_start), 0);

    uiCheckbox *chk_startup = uiNewCheckbox(S(AUTO_STARTUP));
    uiCheckboxSetChecked(chk_startup, settings.start_with_system);
    uiCheckboxOnToggled(chk_startup, chk_startup_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_startup), 0);

    uiCheckbox *chk_mini = uiNewCheckbox(S(SETTINGS_UI_MINI_ROSTER));
    uiCheckboxSetChecked(chk_mini, settings.use_mini_flist);
    uiCheckboxOnToggled(chk_mini, chk_mini_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_mini), 0);

    return uiControl(vbox);
}

static void chk_ptt_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.push_to_talk = uiCheckboxChecked(checkbox);
    if (!settings.push_to_talk) {
        init_ptt();
    } else {
        exit_ptt();
    }
}

static void chk_filtering_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.audiofilter_enabled = uiCheckboxChecked(checkbox);
}

static void cbo_inputs_on_select(uiCombobox *combobox, void *UNUSED(data)){
    int selected = uiComboboxSelected(combobox);
}

static void cbo_outputs_on_select(uiCombobox *combobox, void *UNUSED(data)){
    int selected = uiComboboxSelected(combobox);
}

static void cbo_video_on_select(uiCombobox *combobox, void *UNUSED(data)){
    int selected = uiComboboxSelected(combobox);
    if (selected == 1) {
        utox_screen_grab_desktop(1);
    } else {
        postmessage_utoxav(UTOXAV_SET_VIDEO_IN, selected, 0, NULL);
    }
}

static void btn_audio_preview_clicked(uiButton *UNUSED(button), void *UNUSED(data)){
    //TODO: implement this
}

static void btn_video_preview_clicked(uiButton *UNUSED(button), void *UNUSED(data)){
    //TODO: implement this
}

static uiControl *settings_av_page(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiCheckbox *chk_ptt = uiNewCheckbox(S(PUSH_TO_TALK));
    uiCheckboxSetChecked(chk_ptt, settings.push_to_talk);
    uiCheckboxOnToggled(chk_ptt, chk_ptt_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_ptt), 0);

    uiCheckbox *chk_filtering = uiNewCheckbox(S(AUDIOFILTERING));
    uiCheckboxSetChecked(chk_filtering, settings.audiofilter_enabled);
    uiCheckboxOnToggled(chk_filtering, chk_filtering_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_filtering), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(AUDIOINPUTDEVICE))), 0);
    uiCombobox *cbo_inputs = uiNewCombobox();
    uiComboboxOnSelected(cbo_inputs, cbo_inputs_on_select, NULL);
    uiBoxAppend(vbox, uiControl(cbo_inputs), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(AUDIOOUTPUTDEVICE))), 0);
    uiCombobox *cbo_outputs = uiNewCombobox();
    uiComboboxOnSelected(cbo_outputs, cbo_outputs_on_select, NULL);
    uiBoxAppend(vbox, uiControl(cbo_outputs), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(VIDEOFRAMERATE))), 0);
    uiEntry *entry_fps = uiNewEntry();
    uiBoxAppend(vbox, uiControl(entry_fps), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(VIDEOINPUTDEVICE))), 0);
    uiCombobox *cbo_video = uiNewCombobox();
    uiComboboxOnSelected(cbo_video, cbo_video_on_select, NULL);
    uiBoxAppend(vbox, uiControl(cbo_video), 0);

    uiBoxAppend(vbox, uiControl(hbox), 0);

    uiButton *btn_audio_preview = uiNewButton(S(AUDIO_PREVIEW));
    uiButtonOnClicked(btn_audio_preview, btn_audio_preview_clicked, NULL);
    uiBoxAppend(hbox, uiControl(btn_audio_preview), 0);

    uiButton *btn_video_preview = uiNewButton(S(VIDEO_PREVIEW));
    uiButtonOnClicked(btn_video_preview, btn_video_preview_clicked, NULL);
    uiBoxAppend(hbox, uiControl(btn_video_preview), 0);

    return uiControl(vbox);
}

static void cbo_group_notification_on_selected(uiCombobox *combobox, void *UNUSED(data)){
    settings.group_notifications = uiComboboxSelected(combobox);
}

static void chk_typing_notifications_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.send_typing_status = uiCheckboxChecked(checkbox);
}

static void chk_status_notifications_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.status_notifications = uiCheckboxChecked(checkbox);
}

static void chk_ringtone_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.ringtone_enabled = uiCheckboxChecked(checkbox);
}

static uiControl *settings_notifications_page(void){
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiBoxAppend(vbox, uiControl(hbox), 0);

    uiCheckbox *chk_ringtone = uiNewCheckbox(S(RINGTONE));
    uiCheckboxSetChecked(chk_ringtone, settings.ringtone_enabled);
    uiCheckboxOnToggled(chk_ringtone, chk_ringtone_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_ringtone), 0);

    uiCheckbox *chk_status_notifications = uiNewCheckbox(S(STATUS_NOTIFICATIONS));
    uiCheckboxSetChecked(chk_status_notifications, settings.status_notifications);
    uiCheckboxOnToggled(chk_status_notifications, chk_status_notifications_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_status_notifications), 0);

    uiCheckbox *chk_typing_notifications = uiNewCheckbox(S(SEND_TYPING_NOTIFICATIONS));
    uiCheckboxSetChecked(chk_typing_notifications, settings.send_typing_status);
    uiCheckboxOnToggled(chk_typing_notifications, chk_typing_notifications_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_typing_notifications), 0);

    uiBoxAppend(vbox, uiControl(uiNewLabel(S(GROUP_NOTIFICATIONS))), 0);
    uiCombobox *cbo_group_notifications = uiNewCombobox();
    uiComboboxAppend(cbo_group_notifications, S(GROUP_NOTIFICATIONS_OFF));
    uiComboboxAppend(cbo_group_notifications, S(GROUP_NOTIFICATIONS_MENTION));
    uiComboboxAppend(cbo_group_notifications, S(GROUP_NOTIFICATIONS_ON));
    uiComboboxSetSelected(cbo_group_notifications, settings.group_notifications);
    uiComboboxOnSelected(cbo_group_notifications, cbo_group_notification_on_selected, NULL);
    uiBoxAppend(vbox, uiControl(cbo_group_notifications), 0);

    return uiControl(vbox);
}

static void chk_ipv6_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.enable_ipv6 = uiCheckboxChecked(checkbox);
}

static void chk_udp_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.enable_udp = uiCheckboxChecked(checkbox);
}

static void chk_proxy_toggle(uiCheckbox *checkbox, void *data){
    uiCheckbox *chk_force_proxy = (uiCheckbox *)data;

    settings.use_proxy = uiCheckboxChecked(checkbox);

    if (!settings.use_proxy) {
        settings.force_proxy = false;
        uiCheckboxSetChecked(chk_force_proxy, 0);
    }

    //TODO: get proxy address and port
    /* memcpy(proxy_address, edit_proxy_ip.data, edit_proxy_ip.length); */
    /* proxy_address[edit_proxy_ip.length] = 0; */

    /* edit_proxy_port.data[edit_proxy_port.length] = 0; */
    /* settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0); */

    tox_settingschanged();
}

static void chk_force_proxy_toggle(uiCheckbox *checkbox, void *data){
    uiCheckbox *chk_udp = (uiCheckbox *) data;

    settings.force_proxy = uiCheckboxChecked(checkbox);

    if (settings.force_proxy) {
        uiCheckboxSetChecked(chk_udp, 0);
        settings.enable_udp = false;
    }

    //TODO: get proxy and port
    /* edit_proxy_port.data[edit_proxy_port.length] = 0; */
    /* settings.proxy_port = strtol((char *)edit_proxy_port.data, NULL, 0); */

    tox_settingschanged();
}

static void chk_update_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.auto_update = uiCheckboxChecked(checkbox);
    updater_start(0);
}

static void chk_block_requests_toggle(uiCheckbox *checkbox, void *UNUSED(data)){
    settings.block_friend_requests = uiCheckboxChecked(checkbox);
}

static uiControl *settings_advanced_page(void) {
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiCheckbox *chk_ipv6 = uiNewCheckbox(S(IPV6));
    uiCheckboxSetChecked(chk_ipv6, settings.enable_ipv6);
    uiCheckboxOnToggled(chk_ipv6, chk_ipv6_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_ipv6), 0);

    uiCheckbox *chk_udp = uiNewCheckbox(S(UDP));
    uiCheckboxSetChecked(chk_udp, settings.enable_udp);
    uiCheckboxOnToggled(chk_udp, chk_udp_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_udp), 0);

    uiCheckbox *chk_proxy = uiNewCheckbox(S(PROXY));
    uiCheckboxSetChecked(chk_proxy, settings.use_proxy);
    uiCheckboxOnToggled(chk_proxy, chk_proxy_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_proxy), 0);

    uiBoxAppend(vbox, uiControl(hbox), 0);

    uiEntry *entry_proxy = uiNewEntry();
    uiEntrySetText(entry_proxy, proxy_address);
    uiBoxAppend(hbox, uiControl(entry_proxy), 0);

    char proxy_port[4];
    snprintf(proxy_port, sizeof(proxy_port), "%u", settings.proxy_port);

    uiEntry *entry_port = uiNewEntry();
    uiEntrySetText(entry_port, proxy_port);
    uiBoxAppend(hbox, uiControl(entry_port), 0);

    uiCheckbox *chk_force_proxy = uiNewCheckbox(S(PROXY_FORCE));
    uiCheckboxSetChecked(chk_force_proxy, settings.force_proxy);
    uiCheckboxOnToggled(chk_force_proxy, chk_force_proxy_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_force_proxy), 0);

    uiCheckbox *chk_update = uiNewCheckbox(S(AUTO_UPDATE));
    uiCheckboxSetChecked(chk_update, settings.auto_update);
    uiCheckboxOnToggled(chk_update, chk_update_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_update), 0);

    uiCheckbox *chk_block_requests = uiNewCheckbox(S(BLOCK_FRIEND_REQUESTS));
    uiCheckboxSetChecked(chk_block_requests, settings.block_friend_requests);
    uiCheckboxOnToggled(chk_block_requests, chk_block_requests_toggle, NULL);
    uiBoxAppend(vbox, uiControl(chk_block_requests), 0);

    //TODO: do show password and nospam buttons

    return uiControl(vbox);
}

uiControl *ui_settings_page(void){
    uiTab *tabs = uiNewTab();

    //TODO: draw header with toxcore information
    uiTabAppend(tabs, S(PROFILE_BUTTON), settings_profile_page());
    uiTabSetMargined(tabs, 0, 1);
    uiTabAppend(tabs, S(USER_INTERFACE_BUTTON), settings_interface_page());
    uiTabSetMargined(tabs, 1, 1);
    uiTabAppend(tabs, S(AUDIO_VIDEO_BUTTON), settings_av_page());
    uiTabSetMargined(tabs, 2, 1);
    uiTabAppend(tabs, S(NOTIFICATIONS_BUTTON), settings_notifications_page());
    uiTabSetMargined(tabs, 3, 1);
    uiTabAppend(tabs, S(ADVANCED_BUTTON), settings_advanced_page());
    uiTabSetMargined(tabs, 4, 1);

    return uiControl(tabs);
}

static void btn_submit_clicked(uiButton *UNUSED(button), void *data){
    uiEntry *entry = (uiEntry *)data;
    entry_password_text = uiEntryText(entry);
}

static void entry_password_on_change(uiEntry *entry, void *UNUSED(data)){
    entry_password_text = uiEntryText(entry);
}

uiControl *ui_password_page(void){
    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);

    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);
    uiBoxAppend(hbox, uiControl(vbox), 0);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    uiBoxAppend(vbox, uiControl(grid), 0);

    uiEntry *entry_password = uiNewPasswordEntry();
    uiEntryOnChanged(entry_password, entry_password_on_change, NULL);
    uiGridAppend(grid, uiControl(entry_password), 0, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

    uiButton *submit = uiNewButton("Submit");
    uiButtonOnClicked(submit, btn_submit_clicked, entry_password);
    uiGridAppend(grid, uiControl(submit), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

    return uiControl(hbox);
}

static uiControl *sidebar_create_friend(FRIEND *f){
    (void) f;
    return NULL;
}

static uiControl *sidebar_create_group(GROUPCHAT *g){
    (void) g;
    return NULL;
}

uiControl *ui_sidebar(void){
    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiBox *vbox = uiNewVerticalBox();
    uiBoxSetPadded(vbox, 1);
    uiBoxAppend(hbox, uiControl(vbox), 0);

    uiGrid *grid = uiNewGrid();
    uiGridSetPadded(grid, 1);
    uiBoxAppend(vbox, uiControl(grid), 0);

    //TODO: add profile picutre, currently libui doesn't expose the uiImage control
    uiLabel *name = uiNewLabel(self.name);
    uiLabel *status = uiNewLabel(self.statusmsg);
    uiGridAppend(grid, uiControl(name), 1, 0, 1, 1, 0, uiAlignFill, 0, uiAlignFill);
    uiGridAppend(grid, uiControl(status), 1, 1, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

    uiButton *btn_contact_sort = uiNewButton(S(FILTER_ALL));
    uiGridAppend(grid, uiControl(btn_contact_sort), 0, 2, 1, 1, 0, uiAlignFill, 0, uiAlignFill);

    for (size_t i = 0; i < self.friend_list_count; i++){
        FRIEND *f = get_friend(i);
        if (!f) {
            continue;
        }

        sidebar_create_friend(f);
    }

    for (size_t i = 0; i < self.groups_list_count; i++) {
        GROUPCHAT *g = get_group(i);
        if (!g) {
            continue;
        }

        sidebar_create_group(g);
    }

    return uiControl(hbox);
}

//TODO: implement message page
uiControl *ui_message_page(void){
    uiBox *hbox = uiNewHorizontalBox();
    uiBoxSetPadded(hbox, 1);

    uiDrawTextLayoutParams params;
    memset(&params, 0, sizeof(uiDrawTextLayoutParams));

    uiDrawTextLayout *layout = uiDrawNewTextLayout(&params);

    return uiControl(hbox);
}

//TODO: Implement splash page
uiControl *ui_splash_page(void){
    return NULL;
}

//TODO: see if this can be simplified
void ui_show_page(AREA area, PAGE_TYPE page){
    if (page == current_page) {
        return;
    }

    uiBox *box = NULL;
    switch (area) {
        case AREA_SIDEBAR:
            box = sidebar_vbox;
            break;
        case AREA_MAIN_PAGE:
            box = main_vbox;
            break;
        default:
            LOG_ERR("UI", "Unknown area.");
            return;
    }

    ui_hide_page(area);

    uiControl *control = NULL;
    switch (page) {
        case PAGE_PASSWORD:
            control = ui_password_page();
            break;
        case PAGE_SETTINGS:
            control = ui_settings_page();
            break;
        case PAGE_MESSAGES:
            control = ui_message_page();
            break;
        case PAGE_SPLASH:
            control = ui_splash_page();
            break;
        default:
            LOG_ERR("UI", "Requested page unknown.");
            return;
    }

    uiBoxAppend(box, control, 1);
    current_page = page;
}

void ui_hide_page(AREA area){
    switch (area) {
        case AREA_SIDEBAR:
            uiBoxDelete(sidebar_vbox, UI_SIDEBAR_INDEX);
            break;
        case AREA_MAIN_PAGE:
            uiBoxDelete(main_vbox, UI_MAIN_INDEX);
            break;
        default:
            LOG_ERR("UI", "Unknown page requested to be hidden.");
            return;
    }
}

bool ui_init(int width, int height){
    uiInitOptions o;
	memset(&o, 0, sizeof (uiInitOptions));

	const char *err = uiInit(&o);
	if (err != NULL) {
        LOG_ERR("UI", "Error initializing ui. Error string: %s", err);
		uiFreeInitError(err);
		return false;
	}

	uiWindow *main_window = uiNewWindow("uTox", width, height, 1);
	uiWindowSetMargined(main_window, 1);
	uiWindowOnClosing(main_window, ui_on_quit, NULL);

	uiOnShouldQuit(ui_should_quit, main_window);

    uiControl *sidebar = ui_sidebar();
    uiControl *settings_page = ui_settings_page();

	uiBox *hbox = uiNewHorizontalBox();
    uiWindowSetChild(main_window, uiControl(hbox));
	uiBoxSetPadded(hbox, 1);

	sidebar_vbox = uiNewVerticalBox();
	uiBoxSetPadded(sidebar_vbox, 1);
	uiBoxAppend(hbox, uiControl(sidebar_vbox), 0);
    uiBoxAppend(sidebar_vbox, sidebar, 0);

	uiBoxAppend(hbox, uiControl(uiNewVerticalSeparator()), 0);

	main_vbox = uiNewVerticalBox();
	uiBoxSetPadded(main_vbox, 1);

	uiBoxAppend(hbox, uiControl(main_vbox), 1);
    uiBoxAppend(main_vbox, uiControl(settings_page), 1);

	uiControlShow(uiControl(main_window));

    current_page = PAGE_SETTINGS;
    ui_initialized = true;

	uiMain();
    LOG_DEBUG("UI", "Calling uiUninit.");
	uiUninit();

    return true;
}
