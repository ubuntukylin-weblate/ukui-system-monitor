/*
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * Authors:
 *  Kobe Lee    xiangli@ubuntukylin.com/kobe24_lixiang@126.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filesystemworker.h"
#include "util.h"

#include <stddef.h>
#include <glibtop/mountlist.h>
#include <glibtop/fsusage.h>
/*For PRIu64*/
#include <inttypes.h>

//extern "C" {
//#include <gio/gio.h>
//}

typedef struct _DISK_INFO
{
    char devname[256];
    char mountdir[256];
    char type[256];
    gint percentage;
    guint64 btotal;
    guint64 bfree;
    guint64 bavail;
    guint64 bused;
    gint valid;
} DISK_INFO;

static void fsusage_stats(const glibtop_fsusage *buf,
              guint64 *bused, guint64 *bfree, guint64 *bavail, guint64 *btotal,
              gint *percentage)
{
    guint64 total = buf->blocks * buf->block_size;

    if (!total) {
        /* not a real device */
        *btotal = *bfree = *bavail = *bused = 0ULL;
        *percentage = 0;
    } else {
        int percent;
        *btotal = total;
        *bfree = buf->bfree * buf->block_size;
        *bavail = buf->bavail * buf->block_size;
        *bused = *btotal - *bfree;
        /* percent = 100.0f * *bused / *btotal; */
        percent = 100 * *bused / (*bused + *bavail);
        *percentage = CLAMP(percent, 0, 100);
    }
}

DISK_INFO add_disk(const glibtop_mountentry *entry, gboolean show_all_fs)
{
    DISK_INFO disk;
    memset(&disk, 0, sizeof(disk));
    disk.valid = 0;
    glibtop_fsusage usage;
    guint64 bused, bfree, bavail, btotal;
    gint percentage;
    glibtop_get_fsusage(&usage, entry->mountdir);
    qDebug()<<entry->mountdir<<"1111111111111111111mountdir";
    qDebug()<<"usage.blocks----"<<usage.blocks;
    if (usage.blocks == 0) {
        qDebug()<<"222222";
        return disk;
    }
    if(strcmp(entry->devname,"none")==0 || strcmp(entry->devname,"tmpfs")==0){
        qDebug()<<"333333";
        return disk;
    }
    if(strstr(entry->type, "tmpfs")) {
        qDebug()<<"444444";
        return disk;
    }
    fsusage_stats(&usage, &bused, &bfree, &bavail, &btotal, &percentage);
    memcpy(disk.devname, entry->devname, strlen(entry->devname));
    memcpy(disk.mountdir, entry->mountdir, strlen(entry->mountdir));
    memcpy(disk.type, entry->type, strlen(entry->type));
    disk.percentage = percentage;
    disk.btotal = btotal;
    disk.bfree = bfree;
    disk.bavail = bavail;
    disk.bused = bused;
    disk.valid = 1;
//    qDebug() << disk.devname<<"how i can get it ";//设备
//    qDebug() << disk.mountdir;//目录
//    qDebug() << disk.type;//类型
//    qDebug() << disk.percentage;
//    qDebug() << g_format_size_full(disk.btotal, G_FORMAT_SIZE_DEFAULT);//总数
//    qDebug() << g_format_size_full(disk.bavail, G_FORMAT_SIZE_DEFAULT);//可用
//    qDebug() << g_format_size_full(disk.bused, G_FORMAT_SIZE_DEFAULT);//已用
    return disk;
}

//void hello(gpointer data)
//{
//    g_print ("Hello World\n");
//}

FileSystemWorker::FileSystemWorker(QObject *parent)
    : QObject(parent)
{
//    GVolumeMonitor * monitor;//GVolumeMonitor不是 thread-default-context aware，因此不能在除了主线程中的其他地方使用????
//    monitor = g_volume_monitor_get();
//    g_signal_connect(monitor, "mount-added", G_CALLBACK(hello), NULL);


//    GVolumeMonitor* monitor = g_volume_monitor_get();
//    GList* mountDeviceList = g_volume_monitor_get_mounts(monitor);
//    GList* it = NULL;
//    for(it = mountDeviceList; it; it = it->next) {
//         qDebug() << "mount device list:" << it->data;
//    }
//    GList* mountVolumeList = g_volume_monitor_get_volumes(monitor);
//    for(it = mountVolumeList; it; it = it->next) {
//         qDebug() << "mount volume list:" << it->data;
//    }
}

FileSystemWorker::~FileSystemWorker()
{
    m_diskInfoList.clear();
}

void FileSystemWorker::onFileSystemListChanged()
{
    QStringList newDiskList;
    glibtop_mountentry *entries;
    glibtop_mountlist mountlist;
    guint i;
    gboolean show_all_fs = TRUE;
    entries = glibtop_get_mountlist(&mountlist, show_all_fs);
    qDebug()<<"number---"<<mountlist.number;
    for (i = 0; i < mountlist.number; i++) {
        DISK_INFO disk = add_disk(&entries[i], show_all_fs);
        if (disk.valid == 1) {
            std::string formatted_dev = make_string(g_strdup(disk.devname));
            QString dev_name = QString::fromStdString(formatted_dev);
            qDebug()<<"dev name wwwjwjwjwjwjw"<<dev_name;
            //QString dev_name = QString(QLatin1String(disk.devname));
            newDiskList.append(dev_name);

            if (!this->isDeviceContains(dev_name)) {
                qDebug()<<"if iiiiiiiiii";
                FileSystemData *info = new FileSystemData(this);
                info->setDevName(dev_name);

                std::string formatted_mountdir(make_string(g_strdup(disk.mountdir)));
                std::string formatted_type(make_string(g_strdup(disk.type)));
                char *totalSize = g_format_size_full(disk.btotal, G_FORMAT_SIZE_DEFAULT);
                char *freeSize = g_format_size_full(disk.bfree, G_FORMAT_SIZE_DEFAULT);
                char *availSize = g_format_size_full(disk.bavail, G_FORMAT_SIZE_DEFAULT);
                char *usedSize = g_format_size_full(disk.bused, G_FORMAT_SIZE_DEFAULT);
                std::string formatted_btotal(make_string(g_strdup(totalSize)));
                std::string formatted_bfree(make_string(g_strdup(freeSize)));
                std::string formatted_bavail(make_string(g_strdup(availSize)));
                std::string formatted_bused(make_string(g_strdup(usedSize)));
                info->updateDiskInfo(QString::fromStdString(formatted_mountdir), QString::fromStdString(formatted_type), QString::fromStdString(formatted_btotal), QString::fromStdString(formatted_bfree), QString::fromStdString(formatted_bavail), QString::fromStdString(formatted_bused), disk.percentage/*QString::number(disk.percentage).append("%")*/);

//                info->updateDiskInfo(QString(QLatin1String(disk.mountdir)), QString(QLatin1String(disk.type)), QString(QLatin1String(g_format_size_full(disk.btotal, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bfree, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bavail, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bused, G_FORMAT_SIZE_DEFAULT))), disk.percentage/*QString::number(disk.percentage).append("%")*/);
                this->addDiskInfo(dev_name, info);
                g_free(totalSize);
                g_free(freeSize);
                g_free(availSize);
                g_free(usedSize);
            }
            else {//update info which had exists
                qDebug()<<"elseelseelseelse";
                FileSystemData *info = this->getDiskInfo(dev_name);
                if (info) {
                    char *totalSize = g_format_size_full(disk.btotal, G_FORMAT_SIZE_DEFAULT);
                    char *freeSize = g_format_size_full(disk.bfree, G_FORMAT_SIZE_DEFAULT);
                    char *availSize = g_format_size_full(disk.bavail, G_FORMAT_SIZE_DEFAULT);
                    char *usedSize = g_format_size_full(disk.bused, G_FORMAT_SIZE_DEFAULT);

                    std::string formatted_mountdir(make_string(g_strdup(disk.mountdir)));
                    std::string formatted_type(make_string(g_strdup(disk.type)));
                    std::string formatted_btotal(make_string(g_strdup(totalSize)));
                    std::string formatted_bfree(make_string(g_strdup(freeSize)));
                    std::string formatted_bavail(make_string(g_strdup(availSize)));
                    std::string formatted_bused(make_string(g_strdup(usedSize)));
                    info->updateDiskInfo(QString::fromStdString(formatted_mountdir), QString::fromStdString(formatted_type), QString::fromStdString(formatted_btotal), QString::fromStdString(formatted_bfree), QString::fromStdString(formatted_bavail), QString::fromStdString(formatted_bused), disk.percentage/*QString::number(disk.percentage).append("%")*/);

                    g_free(totalSize);
                    g_free(freeSize);
                    g_free(availSize);
                    g_free(usedSize);
//                    info->updateDiskInfo(QString(QLatin1String(disk.mountdir)), QString(QLatin1String(disk.type)), QString(QLatin1String(g_format_size_full(disk.btotal, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bfree, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bavail, G_FORMAT_SIZE_DEFAULT))), QString(QLatin1String(g_format_size_full(disk.bused, G_FORMAT_SIZE_DEFAULT))), disk.percentage/*QString::number(disk.percentage).append("%")*/);
                }
            }
        }
    }

    //remove the device whice not exists anymore
    for (auto device : m_diskInfoList.keys()) {
        bool foundDevice = false;
        for (auto devName : newDiskList) {
            if (devName == device) {
                foundDevice = true;
                break;
            }
        }

        if (!foundDevice) {
            m_diskInfoList.remove(device);//or erase???
        }
    }

    g_free(entries);
}

FileSystemData *FileSystemWorker::getDiskInfo(const QString &devname)
{
    return m_diskInfoList.value(devname, nullptr);
}

QList<FileSystemData *> FileSystemWorker::diskInfoList() const
{
    return m_diskInfoList.values();
}

void FileSystemWorker::addDiskInfo(const QString &devname, FileSystemData *info)
{
    if (!m_diskInfoList.contains(devname)) {
        m_diskInfoList[devname] = info;
    }
}

void FileSystemWorker::removeDiskItem(const QString &devname)
{
//    FileSystemData *info = getDiskInfo(devname);
//    m_diskInfoList.remove(devname);
}

bool FileSystemWorker::isDeviceContains(const QString &devname)
{
    return m_diskInfoList.keys().contains(devname);
}
