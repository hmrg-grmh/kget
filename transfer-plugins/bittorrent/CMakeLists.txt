#Set supported mime type
SET(SUPPORTED_KGET_MIMETYPES "${SUPPORTED_KGET_MIMETYPES}application/x-bittorrent;" PARENT_SCOPE)

kde_enable_exceptions()

include_directories(
    ../../
)
set(kget_bittorrentfactory_PART_SRCS
    bttransfer.cpp
    bttransferfactory.cpp
    bttransferhandler.cpp
    btdetailswidget.cpp
    scandlg.cpp
    #btdatasource.cpp
    #btchunkselector.cpp
    #btcache.cpp
    advanceddetails/btadvanceddetailswidget.cpp
    advanceddetails/chunkdownloadmodel.cpp
    advanceddetails/chunkdownloadview.cpp
    advanceddetails/fileview.cpp
    advanceddetails/iwfilelistmodel.cpp
    advanceddetails/iwfiletreemodel.cpp
    advanceddetails/peerview.cpp
    advanceddetails/peerviewmodel.cpp
    advanceddetails/torrentfilelistmodel.cpp
    advanceddetails/torrentfilemodel.cpp
    advanceddetails/torrentfiletreemodel.cpp
    advanceddetails/monitor.cpp
    advanceddetails/trackerview.cpp
    advanceddetails/trackermodel.cpp
    advanceddetails/webseedsmodel.cpp
    advanceddetails/webseedstab.cpp
    ../../kget_debug.cpp
    )

ki18n_wrap_ui(kget_bittorrentfactory_PART_SRCS
    btdetailswidgetfrm.ui
    scandlg.ui
    advanceddetails/chunkdownloadview.ui
    advanceddetails/trackerview.ui
    advanceddetails/webseedstab.ui
    )

kconfig_add_kcfg_files(kget_bittorrentfactory_PART_SRCS bittorrentsettings.kcfgc)
add_library(kget_bittorrent MODULE ${kget_bittorrentfactory_PART_SRCS})
kcoreaddons_desktop_to_json(kget_bittorrent kget_bittorrentfactory.desktop)

target_link_libraries(kget_bittorrent KF5::KIOCore KF5::Torrent kgetcore)
install(TARGETS kget_bittorrent DESTINATION  ${KGET_PLUGIN_INSTALL_DIR})

###Build KCM-Module
set(kcm_kget_bittorrentfactory_PART_SRCS
    btsettingswidget.cpp
    ../../kget_debug.cpp
    )

ki18n_wrap_ui(kcm_kget_bittorrentfactory_PART_SRCS
    btsettingswidget.ui
    )

kconfig_add_kcfg_files(kcm_kget_bittorrentfactory_PART_SRCS bittorrentsettings.kcfgc)
add_library(kcm_kget_bittorrentfactory MODULE ${kcm_kget_bittorrentfactory_PART_SRCS})
target_link_libraries(kcm_kget_bittorrentfactory KF5::ConfigGui KF5::ConfigWidgets KF5::I18n KF5::KIOCore KF5::KIOWidgets)
install(TARGETS kcm_kget_bittorrentfactory DESTINATION ${KDE_INSTALL_PLUGINDIR})

install(FILES kget_bittorrentfactory_config.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
