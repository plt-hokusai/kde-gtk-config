/* KDE GTK Configuration Module
 * 
 * Copyright 2011 José Antonio Sanchez Reynaga <joanzare@gmail.com>
 * Copyright 2011 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public 
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "appearancegtk3.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>

QStringList AppearanceGTK3::installedThemes() const
{
    QFileInfoList availableThemes;
    foreach(const QString& themesDir, QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "themes", QStandardPaths::LocateDirectory)) {
        QDir root(themesDir);
        availableThemes += root.entryInfoList(QDir::NoDotAndDotDot|QDir::AllDirs);
    }

    //Also show the user-installed themes
    QDir user(QDir::homePath()+"/.themes");
    availableThemes += user.entryInfoList(QDir::NoDotAndDotDot|QDir::AllDirs);

    //we just want actual themes
    QStringList themes;
    for(QFileInfoList::iterator it=availableThemes.begin(); it!=availableThemes.end(); ++it) {
        bool hasGtkrc = QDir(it->filePath()).exists("gtk-3.0");

        //If it doesn't exist, we don't want it on the list
        if(hasGtkrc)
            themes += it->filePath();
    }

    return themes;
}

bool AppearanceGTK3::saveSettings(const QString& file) const
{
    //Opening GTK3 config file $ENV{XDG_CONFIG_HOME}/gtk-3.0/m_settings.ini
    QDir::home().mkpath(file.left(file.lastIndexOf('/'))); //we make sure the path exists
    QFile file_gtk3(file);
    
    if(!file_gtk3.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open GTK3 config file for writing at:" << file_gtk3.fileName();
        return false;
    }
    QTextStream flow3(&file_gtk3);
    flow3 << "[Settings]\n";
    flow3 << "gtk-font-name=" << m_settings["font"] << "\n";
    flow3 << "gtk-theme-name=" << m_settings["theme"] << "\n";
    flow3 << "gtk-icon-theme-name="<< m_settings["icon"] << "\n";
    flow3 << "gtk-fallback-icon-theme=" << m_settings["icon_fallback"] << "\n";
    flow3 << "gtk-toolbar-style=" << m_settings["toolbar_style"] << "\n";
    flow3 << "gtk-menu-images=" << m_settings["show_icons_menus"] << "\n";
    flow3 << "gtk-button-images=" << m_settings["show_icons_buttons"] << "\n";
    return true;
}

bool AppearanceGTK3::loadSettings(const QString& path)
{
    QFile fileGtk3(path);
    bool canRead=fileGtk3.open(QIODevice::ReadOnly | QIODevice::Text);
    
    if(canRead) {
        const QMap<QString, QString> foundSettings = readSettingsTuples(&fileGtk3);
        
        m_settings = QMap<QString, QString> {
            {"toolbar_style", "GTK_TOOLBAR_ICONS"},
            {"show_icons_buttons", "0"},
            {"show_icons_menus", "0"}
        };

        for(auto it = foundSettings.constBegin(), itEnd = foundSettings.constEnd(); it!=itEnd; ++it) {
            if (it.key() == "gtk-theme-name")
                m_settings["theme"] = *it;
            else if (it.key() == "gtk-icon-theme-name")
                m_settings["icon"] = *it;
            else if (it.key() == "gtk-fallback-icon-theme")
                m_settings["icon_fallback"] = *it;
            else if (it.key() == "gtk-font-name")
                m_settings["font"] = *it;
            else if (it.key() == "gtk-toolbar-style")
                m_settings["toolbar_style"] = *it;
            else if (it.key() == "gtk-button-images")
                m_settings["show_icons_buttons"] = *it;
            else if (it.key() == "gtk-menu-images")
                m_settings["show_icons_menus"] = *it;
            else
                qWarning() << "unknown field" << it.key();
        }
    } else
        qWarning() << "Cannot open the GTK3 config file" << path;
    
    return canRead;
}

QString AppearanceGTK3::defaultConfigFile() const
{
    QString root = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if(root.isEmpty())
        root = QFileInfo(QDir::home(), ".config").absoluteFilePath();
    
    return root+"/gtk-3.0/settings.ini";
}
