#include "ui_about_simple.hpp"

namespace ui {

// TODO: Generate this automatically from github
// Information: a line starting with a '#' will be yellow coloured
const std::string authors_list[] = {
    "#   * List of contributors *  ",
    " ",
    "#Mayhem:",
    "eried,euquiq,gregoryfenton",
    "johnelder,jwetzell,nnemanjan00",
    "N0vaPixel,klockee,gullradriel",
    "jamesshao8,ITAxReal,rascafr",
    "mcules,dqs105,strijar,fossum",
    "zhang00963,RedFox-Fr,aldude999",
    "East2West,ArjanOnwezen,jLynx",
    "vXxOinvizioNxX,teixeluis,n0xa",
    "Brumi-2021,texasyojimbo,zigad",
    "heurist1,intoxsick,ckuethe",
    "notpike,MichalLeonBorsuk",
    "kallanreed,bernd-herzog,zxkmm",
    "NotherNgineer,jimilinuxguy",
    "iNetro,HTotoo,BehleZebub,dc2dc",
    "arneluehrs,u-foka,Maescool",
    "nnesetto,formtapez,F33RNI",
    "rusty-labs,andrej-mk,taskinen",
    "MatiasFernandez,TQMatvey,KimIV",
    "RobertoD91,RndmNmbr,OpCode1300",
    "alex10791,GitClo,GermanAizek",
    "JimiHFord,jstockdale,aSmig",
    "KillerTurtleSoftware,Lpd738",
    "MattLodge,z3r0l1nk,joyel24",
    " ",
    "#Havoc:",
    "furrtek,mrmookie,NotPike",
    "mjwaxios,ImDroided,Giorgiofox",
    "F4GEV,z4ziggy,xmycroftx",
    "troussos,silascutler",
    "nickbouwhuis,msoose,leres",
    "joakar,dhoetger,clem-42",
    "brianlechthaler,ZeroChaos-...",
    " ",
    "#PortaPack:",
    "jboone,argilo",
    " ",
    "#HackRF:",
    "mossmann,dominicgs,bvernoux",
    "bgamari,schneider42,miek",
    "willcode,hessu,Sec42",
    "yhetti,ckuethe,smunaut",
    "wishi,mrbubble62,scateu..."};

AboutView::AboutView(NavigationView& nav) {
    add_children({&menu_view,
                  &button_ok});

    button_ok.on_select = [&nav](Button&) {
        nav.pop();
    };

    menu_view.on_left = [this]() {
        button_ok.focus();
    };

    menu_view.on_right = [this]() {
        button_ok.focus();
    };

    for (const std::string& authors_line : authors_list) {
        // if it's starting with #, it's a title and we have to substract the '#' and paint yellow
        if (authors_line.size() > 0) {
            if (authors_line[0] == '#') {
                menu_view.add_item(
                    {authors_line.substr(1, authors_line.size() - 1),
                     ui::Theme::getInstance()->fg_yellow->foreground,
                     nullptr,
                     nullptr});
            } else {
                menu_view.add_item(
                    {authors_line,
                     Theme::getInstance()->bg_darkest->foreground,
                     nullptr,
                     nullptr});
            }
        }
    }
}

void AboutView::focus() {
    menu_view.focus();
    // put focus on last text line to make it more obvious that list is scrollable
    menu_view.set_highlighted(10);
}

} /* namespace ui */
