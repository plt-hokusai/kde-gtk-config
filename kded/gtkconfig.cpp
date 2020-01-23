/*
 * Copyright (C) 2019 Mikhail Zolotukhin <zomial@protonmail.com>
 * Copyright (C) 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFont>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QGuiApplication>

#include <KIconLoader>
#include <KPluginFactory>
#include <KConfigWatcher>

#include "gtkconfig.h"
#include "configvalueprovider.h"
#include "themepreviewer.h"

K_PLUGIN_CLASS_WITH_JSON(GtkConfig, "gtkconfig.json")

GtkConfig::GtkConfig(QObject *parent, const QVariantList&) :
    KDEDModule(parent),
    configValueProvider(new ConfigValueProvider()),
    themePreviewer(new ThemePreviewer(this)),
    kdeglobalsConfigWatcher(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kdeglobals")))),
    kwinConfigWatcher(KConfigWatcher::create(KSharedConfig::openConfig(QStringLiteral("kwinrc"))))
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerService(QStringLiteral("org.kde.GtkConfig"));
    dbus.registerObject(QStringLiteral("/GtkConfig"), this, QDBusConnection::ExportScriptableSlots);

    connect(qGuiApp, &QGuiApplication::fontChanged, this, &GtkConfig::setFont);
    connect(KIconLoader::global(), &KIconLoader::iconChanged, this, &GtkConfig::setIconTheme);
    connect(kdeglobalsConfigWatcher.data(), &KConfigWatcher::configChanged, this, &GtkConfig::onKdeglobalsSettingsChange);
    connect(kwinConfigWatcher.data(), &KConfigWatcher::configChanged, this, &GtkConfig::onKWinSettingsChange);
    dbus.connect(
        QString(),
        QStringLiteral("/KGlobalSettings"),
        QStringLiteral("org.kde.KGlobalSettings"),
        QStringLiteral("notifyChange"),
        this,
        SLOT(onGlobalSettingsChange(int,int))
    );

    ConfigEditor::removeLegacyGtk2Strings();
    applyAllSettings();
}

GtkConfig::~GtkConfig()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService(QStringLiteral("org.kde.GtkConfig"));
    dbus.unregisterObject(QStringLiteral("/GtkConfig"));
}

void GtkConfig::setGtk2Theme(const QString &themeName) const
{
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-theme-name"), themeName);
}

void GtkConfig::setGtk3Theme(const QString &themeName) const
{
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("gtk-theme"), themeName);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-theme-name"), themeName);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Net/ThemeName"),  themeName);
}

QString GtkConfig::gtk2Theme() const
{
    return ConfigEditor::gtk2ConfigValue(QStringLiteral("gtk-theme-name"));
}

QString GtkConfig::gtk3Theme() const
{
    return ConfigEditor::gtk3ConfigValueSettingsIni(QStringLiteral("gtk-theme-name"));
}

void GtkConfig::showGtk2ThemePreview(const QString& themeName) const
{
    themePreviewer->showGtk2App(themeName);
}

void GtkConfig::showGtk3ThemePreview(const QString& themeName) const
{
    themePreviewer->showGtk3App(themeName);
}

void GtkConfig::setFont() const
{
    const QString configFontName = configValueProvider->fontName();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-font-name"), configFontName);
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("font-name"), configFontName);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-font-name"), configFontName);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/FontName"),  configFontName);
}

void GtkConfig::setIconTheme(int iconGroup) const
{
    if (iconGroup == KIconLoader::Group::Desktop) { // This is needed to update icons only once
        const QString iconThemeName = configValueProvider->iconThemeName();
        ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-icon-theme-name"), iconThemeName);
        ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("icon-theme"), iconThemeName);
        ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-icon-theme-name"), iconThemeName);
        ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Net/IconThemeName"),  iconThemeName);
    }
}

void GtkConfig::setCursorTheme() const
{
    const QString cursorThemeName = configValueProvider->cursorThemeName();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-cursor-theme-name"), cursorThemeName);
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("cursor-theme"), cursorThemeName);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-cursor-theme-name"), cursorThemeName);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/CursorThemeName"),  cursorThemeName);
}

void GtkConfig::setIconsOnButtons() const
{
    const QString iconsOnButtonsConfigValue = configValueProvider->iconsOnButtons();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-button-images"), iconsOnButtonsConfigValue);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-button-images"), iconsOnButtonsConfigValue);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/ButtonImages"), iconsOnButtonsConfigValue);
}

void GtkConfig::setIconsInMenus() const
{
    const QString iconsInMenusConfigValue = configValueProvider->iconsInMenus();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-menu-images"), iconsInMenusConfigValue);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-menu-images"), iconsInMenusConfigValue);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/MenuImages"), iconsInMenusConfigValue);
}

void GtkConfig::setToolbarStyle() const
{
    using ToolbarStyleNotation = ConfigValueProvider::ToolbarStyleNotation;

    QString toolbarStyleSettingsIni = configValueProvider->toolbarStyle(ToolbarStyleNotation::SettingsIni);
    QString toolbarStyleDConf = configValueProvider->toolbarStyle(ToolbarStyleNotation::Dconf);
    QString toolbarStyleXSettingsd = configValueProvider->toolbarStyle(ToolbarStyleNotation::Xsettingsd);

    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-toolbar-style"), toolbarStyleSettingsIni);
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("toolbar-style"), toolbarStyleDConf);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-toolbar-style"), toolbarStyleSettingsIni);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/ToolbarStyle"),  toolbarStyleXSettingsd);
}

void GtkConfig::setScrollbarBehavior() const
{
    const QString scrollbarBehavior = configValueProvider->scrollbarBehavior();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-primary-button-warps-slider"), scrollbarBehavior);
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-primary-button-warps-slider"), scrollbarBehavior);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/PrimaryButtonWarpsSlider"), scrollbarBehavior);
}

void GtkConfig::setDarkThemePreference() const
{
    const QString preferDarkTheme = configValueProvider->preferDarkTheme();
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-application-prefer-dark-theme"), preferDarkTheme);
}

void GtkConfig::setWindowDecorationsButtonsOrder() const
{
    const QString windowDecorationsButtonOrder = configValueProvider->windowDecorationsButtonsOrder();
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("button-layout"), windowDecorationsButtonOrder, QStringLiteral("org.gnome.desktop.wm.preferences"));
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-decoration-layout"), windowDecorationsButtonOrder);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/DecorationLayout"), windowDecorationsButtonOrder);
}

void GtkConfig::setEnableAnimations() const
{
    const QString enableAnimations = configValueProvider->enableAnimations();
    ConfigEditor::setGtk2ConfigValue(QStringLiteral("gtk-enable-animations"), enableAnimations);
    // FIXME work with booleans in configValueProvider...
    ConfigEditor::setGtk3ConfigValueDconf(QStringLiteral("enable-animations"), enableAnimations == QLatin1String("1"));
    ConfigEditor::setGtk3ConfigValueSettingsIni(QStringLiteral("gtk-enable-animations"), enableAnimations);
    ConfigEditor::setGtk3ConfigValueXSettingsd(QStringLiteral("Gtk/EnableAnimations"), enableAnimations);
}

void GtkConfig::applyAllSettings() const
{
    setFont();
    setIconTheme(KIconLoader::Group::Desktop);
    setCursorTheme();
    setIconsOnButtons();
    setIconsInMenus();
    setToolbarStyle();
    setScrollbarBehavior();
    setDarkThemePreference();
    setWindowDecorationsButtonsOrder();
    setEnableAnimations();
}

void GtkConfig::onGlobalSettingsChange(int settingsChangeType, int arg) const
{
    SettingsChangeType changeType = static_cast<SettingsChangeType>(settingsChangeType);
    SettingsCategory settingsCategory = static_cast<SettingsCategory>(arg);

    if (changeType == SettingsChangeType::Cursor) {
        setCursorTheme();
    } else if (changeType == SettingsChangeType::Settings && settingsCategory == SettingsCategory::Style) {
        setIconsOnButtons();
        setIconsInMenus();
        setToolbarStyle();
    } else if (changeType == SettingsChangeType::Settings && settingsCategory == SettingsCategory::Mouse) {
        setScrollbarBehavior();
    } else if (changeType == SettingsChangeType::Palette) {
        setDarkThemePreference();
    }
}

void GtkConfig::onKdeglobalsSettingsChange(const KConfigGroup &group, const QByteArrayList &names) const
{
    if (group.name() == QLatin1String("KDE")
            && names.contains(QByteArrayLiteral("AnimationDurationFactor"))) {
        setEnableAnimations();
    }
}

void GtkConfig::onKWinSettingsChange(const KConfigGroup &group, const QByteArrayList &names) const
{
    if (group.name() == QStringLiteral("org.kde.kdecoration2")
            && (names.contains("ButtonsOnRight") || names.contains("ButtonsOnLeft"))) {
        setWindowDecorationsButtonsOrder();
    }
}

#include "gtkconfig.moc"
