#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

struct TurbineWorld {
  QString name;
  QString chatServer;
  QUrl statusUrl;
  int order;
};

// What a Turbine game's own launcher does before the client starts, read from the game folder's launcher
// configuration: the GLS datacenter service names the worlds and the auth server; the account logs in for a
// ticket; the world's status names its login server and queue; a number is taken in that queue; and the launcher's
// server-side template becomes the client's command line. Nothing is kept: the ticket lives in the arguments only.
class TurbineLauncher : public QObject {
  Q_OBJECT

public:
  explicit TurbineLauncher(QNetworkAccessManager* network, QObject* parent = nullptr);

  void listWorlds(const QString& gameFolder, const QString& launcherConfig);
  void prepareLaunch(const QString& gameFolder, const QString& launcherConfig, const QString& worldName,
    const QString& username, const QString& password, const QString& language);

signals:
  void worldsListed(const QList<TurbineWorld>& worlds);
  void statusChanged(const QString& text);
  void launchReady(const QString& executable, const QStringList& arguments);
  void failed(const QString& message);

private:
  struct Datacenter {
    QUrl authServer;
    QUrl serverConfigUrl;
    QList<TurbineWorld> worlds;
  };
  struct LauncherConfig {
    QUrl glsService;
    QString gameName;
  };

  void fetch(const QUrl& url, const QByteArray& body, const QString& contentType, const QString& soapAction,
    const std::function<void(const QByteArray&)>& onAnswer);
  void loadDatacenter(const LauncherConfig& config, const std::function<void(const Datacenter&)>& onLoaded);
  bool readLauncherConfig(const QString& gameFolder, const QString& launcherConfig, LauncherConfig& config);

  QNetworkAccessManager* network;
};
