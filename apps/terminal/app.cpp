#include "app.h"
#include "terminal_icon.h"
#include "apps/apps_container.h"
#include "apps/i18n.h"


namespace terminal 
{

I18n::Message App::Descriptor::name() 
{
  return I18n::Message::TerminalApp;
}

I18n::Message App::Descriptor::upperName() 
{
  return I18n::Message::TerminalAppCapital;
}

const Image * App::Descriptor::icon() 
{
  return ImageStore::TerminalIcon;
}

App * App::Snapshot::unpack(Container * container) 
{
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor * App::Snapshot::descriptor()
{
  static Descriptor descriptor;
  return &descriptor;
}

App::App(Snapshot * snapshot) :
  ::App(snapshot, nullptr)
{
}

}