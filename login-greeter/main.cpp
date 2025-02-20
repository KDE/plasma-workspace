#include <QGuiApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QScreen>

class LoginGreeter : public QObject
{
    Q_OBJECT
public:
    explicit LoginGreeter(QObject *parent = nullptr)
        : QObject(parent)
    {
        connect(qApp, &QGuiApplication::screenAdded, this, [this](QScreen *screen) {
            createWindowForScreen(screen);
        });
        for (QScreen *screen : qApp->screens()) {
            createWindowForScreen(screen);
        }
    }

private:
    void createWindowForScreen(QScreen *screen)
    {
        QQuickView *window = new QQuickView();
        window->QObject::setParent(this);
        window->setScreen(screen);
        window->showFullScreen();
        window->setSource(QUrl("qrc:/main.qml"));
        connect(qApp, &QGuiApplication::screenRemoved, this, [this, window, screen](QScreen *screenRemoved) {
            if (screenRemoved == screen) {
                delete window;
            }
        });
    }

private Q_SLOTS:
    void onScreenAdded(QScreen *screen)
    {
        createWindowForScreen(screen);
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    LoginGreeter greeter;
    return app.exec();
}

#include "main.moc"
