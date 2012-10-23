#include "settings.h"
#include <QVariant>
#include <QMetaObject>
#include <QMetaProperty>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QtCore>
#include "logger.h"
#include "directorymanager.h"
#include "yaml-cpp/yaml.h"

Settings* Settings::m_spInstance = NULL;

#define CONFIG_UPDATE_INTERVAL "CONFIG_UPDATE_INTERVAL"

#define LAUNCHER "LAUNCHER"
#define LAUNCHER_UPDATE_URL "LAUNCHER_UPDATE_URL"
#define LAUNCHER_VER "LAUNCHER_VER"

#define STABLE "STABLE"
#define DEVELOPMENT "DEVELOPMENT"
#define DEPENDENCIES "DEPENDENCIES"

#define VERSION "ver"
#define RUN_PATH "runPath"
#define INSTALL_CMD "installCmd"
#define UNINSTALL_CMD "uninstallCmd"
#define INSTALLED_FILES "installedFiles"
#define INSTALL_PARAMS "installParams"
#define UNINSTALL_PARAMS "uninstallParams"

#define InitFile "/settings.yaml"

Settings* Settings::GetInstance()
{
    if (!m_spInstance)
        m_spInstance = new Settings();

    return m_spInstance;
}

Settings::Settings(QObject *parent) :
    QObject(parent) {

}

void Settings::Init() {
    //init properties
    //SetProperty(CONFIG_UPDATE_INTERVAL, 30 * 60 * 1000); //30 minutes default interval
    //SetProperty(LAUCHER_VER, "0.0");

    m_nUpdateTimer = 30 * 60 * 1000;
    m_Config.m_Launcher.m_Version = "0.0";

    ParseInitConfig();
}

void Settings::ParseInitConfig() {
    QFile file(DirectoryManager::GetInstance()->GetConfigDir() + InitFile);
    if (!file.open(QFile::ReadOnly)) {
        Logger::GetInstance()->AddLog(tr("Error read config"));
        return;
    }

    YAML::Parser parser;
    std::istringstream stream(file.readAll().data());
    parser.Load(stream);

    file.close();

    YAML::Node doc;
    if (!parser.GetNextDocument(doc)) {
        Logger::GetInstance()->AddLog(tr("Error parse settings."));
        return;
    }

#define setString(a, b) {std::string c; b->GetScalar(c); a = c.c_str();}
    const YAML::Node* pTimer = doc.FindValue(CONFIG_UPDATE_INTERVAL);
    if (pTimer) {
        QString interval;
        setString(interval, pTimer);
        m_nUpdateTimer = interval.toInt() * 60 * 1000;
    }
    AppsConfig newConfig;
    const YAML::Node* Launcher = doc.FindValue(LAUNCHER);
    if (Launcher) {
        const YAML::Node* LauncherVer = Launcher->FindValue(LAUNCHER_VER);
        if (LauncherVer)
            setString(newConfig.m_Launcher.m_Version, LauncherVer);
        const YAML::Node* LauncherUrl = Launcher->FindValue(LAUNCHER_UPDATE_URL);
        if (LauncherUrl)
            setString(newConfig.m_Launcher.m_Url, LauncherUrl);
    }

    ParseAppConfig(&doc, STABLE, newConfig.m_Stable);
    ParseAppConfig(&doc, DEVELOPMENT, newConfig.m_Development);
    ParseAppConfig(&doc, DEPENDENCIES, newConfig.m_Dependencies);
    m_Config = newConfig;

}

void Settings::ParseAppConfig(const YAML::Node* pNode, const char* appType, AppsConfig::AppMap& appMap) {
    const YAML::Node* pBaseNode = pNode->FindValue(appType);
    if (pBaseNode) {
        for (YAML::Iterator iter = pBaseNode->begin(); iter != pBaseNode->end(); ++iter) {
#define setString(a, b) {std::string c; b->GetScalar(c); a = c.c_str();}
            AppConfig config;
            const YAML::Node* pName = &iter.first();
            if (pName) setString(config.m_Name, pName);
            const YAML::Node& appNode = iter.second();
            const YAML::Node* pVer = appNode.FindValue(VERSION);
            if (pVer) setString(config.m_Version, pVer);
            const YAML::Node* pRunPath = appNode.FindValue(RUN_PATH);
            if (pRunPath) setString(config.m_RunPath, pRunPath);
            const YAML::Node* pInstallCmd = appNode.FindValue(INSTALL_CMD);
            if (pInstallCmd) setString(config.m_InstallCmd, pInstallCmd);
            const YAML::Node* pUninstallCmd = appNode.FindValue(UNINSTALL_CMD);
            if (pUninstallCmd) setString(config.m_UninstallCmd, pUninstallCmd);
            const YAML::Node* pInstallParams = appNode.FindValue(INSTALL_PARAMS);
            if (pInstallParams) setString(config.m_InstallParams, pInstallParams);
            const YAML::Node* pUninstallParams = appNode.FindValue(UNINSTALL_PARAMS);
            if (pUninstallParams) setString(config.m_UninstallParams, pUninstallParams);

            const YAML::Node* pInstalledFiles = appNode.FindValue(INSTALLED_FILES);
            if (pInstalledFiles) {
                for (YAML::Iterator iter = pInstalledFiles->begin();
                     iter != pInstalledFiles->end();
                     ++iter) {
                    std::string a;
                    *iter >> a;
                    config.m_InstalledFiles.push_back(a.c_str());
                }
            }

            appMap[config.m_Name] = config;
        }
    }
}

void Settings::UpdateInitConfig() {
    YAML::Emitter emitter;
    emitter << YAML::BeginMap;
    //emitter << YAML::Key << CONFIG_UPDATE_INTERVAL;
    //emitter << YAML::Value << m_nUpdateTimer;

    emitter << YAML::Key << LAUNCHER;
    std::map<std::string, std::string> launcher;
    launcher[QString(LAUNCHER_VER).toStdString()] = m_Config.m_Launcher.m_Version.toStdString();
    launcher[QString(LAUNCHER_UPDATE_URL).toStdString()] = m_Config.m_Launcher.m_Url.toString().toStdString();
    emitter << YAML::Value << launcher;

    EmitAppConfig(emitter, STABLE, m_Config.m_Stable);
    EmitAppConfig(emitter, DEVELOPMENT, m_Config.m_Development);
    EmitAppConfig(emitter, DEPENDENCIES, m_Config.m_Dependencies);

    emitter << YAML::EndMap;

    QString path = DirectoryManager::GetInstance()->GetConfigDir() + InitFile;
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        Logger::GetInstance()->AddLog(tr("Error save config"));
        return;
    }

    file.write(emitter.c_str());

    file.close();
}

void Settings::EmitAppConfig(YAML::Emitter& emitter, const char* appType, const AppsConfig::AppMap& appsMap) {
    if (appsMap.size()) {
        emitter << YAML::Key << appType;
        emitter << YAML::Value;
        emitter << YAML::BeginMap;
        for (AppsConfig::AppMap::const_iterator iter = appsMap.begin(); iter!= appsMap.end(); ++iter) {
            const AppConfig& config = iter.value();
            emitter << YAML::Key << config.m_Name.toStdString();
            emitter << YAML::Value;
            emitter << YAML::BeginMap;
            emitter << YAML::Key << VERSION;
            emitter << YAML::Value << config.m_Version.toStdString();
            emitter << YAML::Key << RUN_PATH;
            emitter << YAML::Value << config.m_RunPath.toStdString();
            emitter << YAML::Key << INSTALL_CMD;
            emitter << YAML::Value << config.m_InstallCmd.toStdString();
            emitter << YAML::Key << UNINSTALL_CMD;
            emitter << YAML::Value << config.m_UninstallCmd.toStdString();
            emitter << YAML::Key << INSTALL_PARAMS;
            emitter << YAML::Value << config.m_InstallParams.toStdString();
            emitter << YAML::Key << UNINSTALL_PARAMS;
            emitter << YAML::Value << config.m_UninstallParams.toStdString();

            if (config.m_InstalledFiles.size()) {
                emitter << YAML::Key << INSTALLED_FILES;
                emitter << YAML::Value;
                emitter << YAML::BeginSeq;
                for (int i = 0; i < config.m_InstalledFiles.size(); i++) {
                    emitter << config.m_InstalledFiles.at(i).toStdString();
                }
                emitter << YAML::EndSeq;
            }
            emitter << YAML::EndMap;
        }
        emitter << YAML::EndMap;
    }

}


void Settings::SetLauncherVersion(const QString& version) {
    m_Config.m_Launcher.m_Version = version;
    UpdateInitConfig();
}

QString Settings::GetLauncherVersion() const {
    return m_Config.m_Launcher.m_Version;
}

void Settings::SetLauncherUrl(const QString& url) {
    m_Config.m_Launcher.m_Url = url;
    UpdateInitConfig();
}

QString Settings::GetLauncherUrl() const {
    return m_Config.m_Launcher.m_Url.toString();
}

int Settings::GetVersion(const QString& strVersion) {
    //"10.123"
    QStringList list = strVersion.split(".");
    if (list.size() != 2) {
        return -1;
    }
    int nVersion = -1;
    nVersion = list.at(0).toInt() << 16;
    nVersion += list.at(1).toInt();
    return nVersion;
}

const AppsConfig& Settings::GetCurrentConfig() const {
    return m_Config;
}

void Settings::UpdateConfig(const AppsConfig& config) {
    m_Config = config;
    UpdateInitConfig();
}
