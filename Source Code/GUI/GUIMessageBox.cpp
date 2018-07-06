#include "Headers/GUIMessageBox.h"

#include <CEGUI/CEGUI.h>

GUIMessageBox::GUIMessageBox(const std::string& id,  
                             const std::string& title, 
                             const std::string& message,
                             const vec2<I32>& offsetFromCentre,  
                             CEGUI::Window* parent) : GUIElement(parent,GUI_MESSAGE_BOX, offsetFromCentre)
{
    // Get a local pointer to the CEGUI Window Manager, Purely for convenience to reduce typing
    CEGUI::WindowManager *pWindowManager = CEGUI::WindowManager::getSingletonPtr();
    // load the messageBox Window from the layout file
    _msgBoxWindow = pWindowManager->loadLayoutFromFile("messageBox.layout");
    _parent->addChild(_msgBoxWindow);
    CEGUI::PushButton* confirmBtn = dynamic_cast<CEGUI::PushButton*>(_msgBoxWindow->getChild("ConfirmBtn"));
    confirmBtn->subscribeEvent(CEGUI::PushButton::EventClicked, CEGUI::Event::Subscriber(&GUIMessageBox::onConfirm, this));

    setTitle(title);
    setMessage(message);
    setOffset(offsetFromCentre);
    setActive(true);
    setVisible(false);
}

GUIMessageBox::~GUIMessageBox(){
    _parent->removeChild(_msgBoxWindow);
}

bool GUIMessageBox::onConfirm(const CEGUI::EventArgs& /*e*/) {
    setActive(false);
    setVisible(false);
    return true;
}

void GUIMessageBox::setVisible(const bool visible) {
    _msgBoxWindow->setVisible(visible);
    _msgBoxWindow->setModalState(visible);
    GUIElement::setVisible(visible);
}

void GUIMessageBox::setActive(const bool active) {
    _msgBoxWindow->setEnabled(active);
    GUIElement::setActive(active);
}

void GUIMessageBox::setTitle(const std::string& titleText) {
    _msgBoxWindow->setText(titleText);
}

void GUIMessageBox::setMessage(const std::string& message) {
    _msgBoxWindow->getChild("MessageText")->setText(message);
}

void GUIMessageBox::setOffset(const vec2<I32>& offsetFromCentre) {
    CEGUI::UVector2 crtPosition(_msgBoxWindow->getPosition());
    crtPosition.d_x.d_offset += offsetFromCentre.x;
    crtPosition.d_y.d_offset += offsetFromCentre.y;
    _msgBoxWindow->setPosition(crtPosition);
}

void GUIMessageBox::setMessageType(MessageType type) {
    switch(type) {
        case MESSAGE_INFO : {
            _msgBoxWindow->setProperty("CaptionColour", "FFFFFFFF");
        } break;
        case MESSAGE_WARNING : {
            _msgBoxWindow->setProperty("CaptionColour", "00FFFFFF");
        } break;
        case MESSAGE_ERROR : {
            _msgBoxWindow->setProperty("CaptionColour", "FF0000FF");
        } break;
    }
}