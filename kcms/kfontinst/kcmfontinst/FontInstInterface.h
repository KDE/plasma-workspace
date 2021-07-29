#pragma once

#include "FontInst.h"
#include "FontinstIface.h"

namespace KFI
{
class FontInstInterface : public OrgKdeFontinstInterface
{
public:
    FontInstInterface()
        : OrgKdeFontinstInterface(OrgKdeFontinstInterface::staticInterfaceName(), FONTINST_PATH, QDBusConnection::sessionBus(), nullptr)
    {
    }
};

}
