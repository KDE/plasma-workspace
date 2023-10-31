#include <QQmlEngine>
#include <QQmlExtensionPlugin>

class ComponentsPlugin : public QQmlExtensionPlugin
{
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
    Q_OBJECT
public:
    void initializeEngine(QQmlEngine *engine, const char *uri) override
    {
        Q_UNUSED(uri)
        engine->setProperty("_kirigamiTheme", QStringLiteral("KirigamiPlasmaStyle"));
    }
    void registerTypes(const char *uri) override
    {
        Q_UNUSED(uri)
    }
};

#include "componentsplugin.moc"
