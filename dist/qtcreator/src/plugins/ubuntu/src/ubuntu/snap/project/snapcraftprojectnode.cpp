/*
 * Copyright 2016 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 2.1.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Benjamin Zeller <benjamin.zeller@canonical.com>
 */

#include "snapcraftprojectnode.h"
#include "snapcraftproject.h"

#include <projectexplorer/nodesvisitor.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <coreplugin/fileiconprovider.h>
#include <ubuntu/ubuntuconstants.h>

#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include <QFileSystemWatcher>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <yaml-cpp/yaml.h>
#pragma GCC diagnostic pop

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};


static QIcon generateIcon(const QString &overlay) {
    const QSize desiredSize = QSize(16, 16);

    const QPixmap overlayPixmap(overlay);
    const QIcon overlayIcon(overlayPixmap.scaled(12, 12));
    const QPixmap pixmap
            = Core::FileIconProvider::overlayIcon(QStyle::SP_DirIcon, overlayIcon, desiredSize);

    QIcon result;
    result.addPixmap(pixmap);

    return result;
}

static QIcon generateProjectIcon () {
    static QIcon projectIcon;
    if (projectIcon.isNull())
        projectIcon = generateIcon(QString::fromLatin1(Constants::UBUNTU_ICON));

    return projectIcon;
}

SnapcraftProjectNode::SnapcraftProjectNode(SnapcraftProject *rootProject, const Utils::FileName &projectFilePath, QFileSystemWatcher *watcher)
    : ProjectExplorer::ProjectNode (projectFilePath),
      m_rootProject(rootProject),
      m_watcher(watcher)
{
    setDisplayName(projectFilePath.parentDir().toFileInfo().fileName());
    setIcon(generateProjectIcon());
}

SnapcraftProjectNode::~SnapcraftProjectNode()
{
}

bool SnapcraftProjectNode::syncFromYAMLNode(YAML::Node rootNode)
{
    if(debug) qDebug()<<"Sync from YAML node";
    try {
        QString displayName = QString::fromStdString(rootNode["name"].as<std::string>());
        setDisplayName(displayName);

        YAML::Node parts = rootNode["parts"];
        if (!parts.IsMap()) {
            ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error,
                                              QString::fromLatin1("Error while parsing snapcraft.yaml: parts is not a map"),
                                              ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                                              m_rootProject->projectFilePath());
            return false;
        }

        QList<ProjectExplorer::FolderNode *> existingNodes = subFolderNodes();
        QList<ProjectExplorer::FolderNode *> nodesToRemove;
        QList<ProjectExplorer::FolderNode *> nodesToAdd;

        QStringList partsFromYaml;
        QStringList existingParts;

        for (const auto &part : existingNodes) {
            existingParts << part->displayName();
        }

        //iterate over the parts
        for (auto it = parts.begin(); it != parts.end(); ++it) {

            YAML::Node subProject = it->second;
            QString partName = QString::fromStdString(it->first.as<std::string>());
            //QString subType  = QString::fromStdString(subProject["plugin"].as<std::string>());
            QString source   = QDir::cleanPath(QString::fromStdString(subProject["source"].as<std::string>()));

            partsFromYaml << partName;

            // We only show a part if it locally exists and is a directory
            QString sourcePathName = QDir::cleanPath(filePath().parentDir().appendPath(source).toString());
            Utils::FileName sourcePath = Utils::FileName::fromString(sourcePathName);
            if (sourcePath.exists() && sourcePath.toFileInfo().isDir()) {

                int idx = existingParts.indexOf(partName);
                if (idx >= 0) {
                    //check if source is still the same
                    SnapcraftGenericPartNode *n = static_cast<SnapcraftGenericPartNode *>(existingNodes.at(idx));
                    if (n->filePath() == sourcePath)
                        continue;

                    nodesToRemove << n;
                }

                SnapcraftGenericPartNode *partNode =  new SnapcraftGenericPartNode(partName, sourcePath, m_watcher);
                nodesToAdd << partNode;
            }
        }


        //snapcraft has magic directories, like setup, we want to show in the project tree
        QStringList magicSnapcraftDirs{
            QStringLiteral("setup")
        };

        for (const QString &magicDir: magicSnapcraftDirs) {
            Utils::FileName dirPath = filePath().parentDir().appendPath(magicDir);
            if (dirPath.exists()) {
                partsFromYaml << magicDir;
                if(!existingParts.contains(magicDir)) {
                    SnapcraftGenericPartNode *partNode =  new SnapcraftGenericPartNode(magicDir, dirPath, m_watcher);
                    nodesToAdd << partNode;
                }
            }
        }

        QSet<QString> obsoleteParts = existingParts.toSet() - partsFromYaml.toSet();

        if(debug) {
            qDebug()<<"Parts in yaml"<<partsFromYaml;
            qDebug()<<"Currently known parts"<<existingParts;
            qDebug()<<"Parts now obsolete: "<<obsoleteParts;
        }

        for (const auto &part : obsoleteParts) {
            int idx = existingParts.indexOf(part);
            if (idx >= 0)
                nodesToRemove << existingNodes.at(idx);
        }

        //remove old nodes
        removeFolderNodes(nodesToRemove);
        addFolderNodes(nodesToAdd);
    } catch (const YAML::Exception &e) {
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::Task::Error,
                                          QString::fromLatin1("Error while parsing snapcraft.yaml: %1").arg(QString::fromLatin1(e.what())),
                                          ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM,
                                          m_rootProject->projectFilePath(),
                                          e.mark.line);
        return false;
    }

    return true;
}

/*!
    \class Ubuntu::Internal::SnapcraftGenericPartNode
    \brief The SnapcraftGenericPartNode class
    Fallback node if the subproject type can not be handled by the plugin directly
 */

static void enumChild(const QDir &dir, QSet<Utils::FileName> &dirs, QSet<Utils::FileName> &res) {
    foreach (const QFileInfo &info, dir.entryInfoList(QDir::NoDotAndDotDot|QDir::Dirs|QDir::Files|QDir::Hidden)) {
        if (info.isDir()) {
            dirs << Utils::FileName(info);
            enumChild(QDir(info.absoluteFilePath()), dirs, res);
        } else {
            res.insert(Utils::FileName(info));
        }
    }
}

SnapcraftGenericPartNode::SnapcraftGenericPartNode(const QString &partName, const Utils::FileName &folderPath, QFileSystemWatcher *watcher)
    : ProjectExplorer::FolderNode (folderPath, ProjectExplorer::FolderNodeType, partName)
    , m_watcher(watcher)
{
    scheduleProjectScan();

    setIcon(generateProjectIcon());

    if (watcher->addPath(folderPath.toString())) {
        if(debug) qDebug()<<"Added"<<folderPath.toString()<<"to watcher";
    } else {
        if(debug) qDebug()<<"Failed to add"<<folderPath.toString()<<"to watcher";
    }
    m_watcherConnection = QObject::connect(watcher, &QFileSystemWatcher::directoryChanged, [this](const QString &path){
        maybeScheduleProjectScan(path);
    });
}

SnapcraftGenericPartNode::~SnapcraftGenericPartNode()
{
    //@BUG the watcher is still watching all the subdirs and files!
    if (m_watcher) {
        QString myPath = filePath().toFileInfo().absoluteFilePath();
        QStringList watched = m_watcher->directories();
        QStringList toRemove;
        for(const QString &path: watched) {
            if(path.startsWith(myPath))
                toRemove << path;
        }
        m_watcher->removePaths(toRemove);
        QObject::disconnect(m_watcherConnection);
    }
}

void SnapcraftGenericPartNode::maybeScheduleProjectScan(const QString &changedPath)
{
    Utils::FileName changed = Utils::FileName::fromString(changedPath);
    if (filePath().toFileInfo() == changed.toFileInfo())
        scheduleProjectScan();
}

void SnapcraftGenericPartNode::scheduleProjectScan()
{
    if (m_scanning)
        return;

    if(debug)  qDebug()<<"Scheduling Project scan";

    m_scanning = true;

    QTimer *rescanTimer = new QTimer();
    rescanTimer->setSingleShot(true);
    rescanTimer->start(0);
    QObject::connect(rescanTimer, &QTimer::timeout, [this, rescanTimer](){
        if(debug) qDebug()<<"Starting Project scan";
        delete rescanTimer;
        this->scanProjectDirectory();

        m_scanning = false;
    });
}

void SnapcraftGenericPartNode::removeFileNodes(const QList<Utils::FileName> &files)
{
    foreach(const Utils::FileName &f, files) {
        if(f.toFileInfo().isDir())
            continue;

        FindFileNodesForFileVisitor vis(f);
        this->accept(&vis);

        for (ProjectExplorer::FileNode *node : vis.nodes()) {
            node->parentFolderNode()->removeFileNodes({node});
        }
    }
}

void SnapcraftGenericPartNode::removeFolderNodes(QList<Utils::FileName> &dirs)
{

    qSort(dirs.begin(), dirs.end(),[](const Utils::FileName &a, const Utils::FileName &b){
        return a.toFileInfo().absoluteFilePath() > b.toFileInfo().absoluteFilePath();
    });

    foreach(const Utils::FileName &f, dirs) {
        FindNodesForFolderVisitor vis(f);
        this->accept(&vis);

        FindNodesForFolderVisitor visParent(f.parentDir());
        this->accept(&visParent);

        if(visParent.nodes().size()) {
            m_watcher->removePath(f.toFileInfo().absoluteFilePath());
            visParent.nodes()[0]->removeFolderNodes(vis.nodes());
        }
    }
}

QList<ProjectExplorer::ProjectAction> SnapcraftGenericPartNode::supportedActions(ProjectExplorer::Node *node) const
{
    static const QList<ProjectExplorer::ProjectAction> fileActions {
        ProjectExplorer::ProjectAction::Rename,
        ProjectExplorer::ProjectAction::RemoveFile
    };
    static const QList<ProjectExplorer::ProjectAction> folderActions {
        ProjectExplorer::ProjectAction::AddNewFile,
        ProjectExplorer::ProjectAction::RemoveFile
    };
    switch (node->nodeType()) {
    case ProjectExplorer::FileNodeType:
        return fileActions;
    case ProjectExplorer::FolderNodeType:
    case ProjectExplorer::ProjectNodeType:
        return folderActions;
    default:
        return ProjectExplorer::FolderNode::supportedActions(node);
    }
}

bool SnapcraftGenericPartNode::addFiles(const QStringList &, QStringList *)
{
    return true;
}

bool SnapcraftGenericPartNode::removeFiles(const QStringList &, QStringList *)
{
    return true;
}

bool SnapcraftGenericPartNode::deleteFiles(const QStringList &)
{
    return true;
}

void SnapcraftGenericPartNode::scanProjectDirectory()
{
    QSet<Utils::FileName> dirs;
    QSet<Utils::FileName> files;
    enumChild(QDir(filePath().toString()), dirs, files);

    FindFilesAndDirsVisitor vis;
    this->accept(&vis);

    QSet<Utils::FileName> oldFiles = QSet<Utils::FileName>::fromList(vis.filePaths());
    QSet<Utils::FileName> oldDirs  = QSet<Utils::FileName>::fromList(vis.dirPaths());
    QSet<Utils::FileName> newFiles(files);
    QSet<Utils::FileName> newDirs(dirs);

    QSet<Utils::FileName> filesToRemove = oldFiles - newFiles;
    QSet<Utils::FileName> filesToAdd    = newFiles - oldFiles;
    QList<Utils::FileName> dirsToRemove  = (oldDirs - newDirs).toList();
    QSet<Utils::FileName> dirsToAdd     = newDirs - oldDirs;

    if(debug) {
        qDebug()<<"Removing dirs " <<dirsToRemove;
        qDebug()<<"Adding dirs   " <<dirsToAdd;
        qDebug()<<"Removing files" <<filesToRemove;
        qDebug()<<"Adding files  " <<filesToAdd;
    }

    removeFileNodes(filesToRemove.toList());
    removeFolderNodes(dirsToRemove);

    for (const Utils::FileName &dir :  dirsToAdd) {
        Utils::FileName relativeName = dir.relativeChildPath(filePath());
        QStringList relativeNameList = relativeName.toUserOutput().split(QDir::separator());
        createOrFindFolder(relativeNameList);
    }

    for (const Utils::FileName &file :  filesToAdd) {
        Utils::FileName folderPath = file.parentDir();

        //do not show the project file twice
        if (file == projectNode()->filePath())
            continue;

        ProjectExplorer::FolderNode *parentNode = nullptr;
        if (folderPath == filePath()) {
            parentNode = this;
        } else {
            Utils::FileName relativeName = folderPath.relativeChildPath(filePath());
            QStringList relativeNameList = relativeName.toUserOutput().split(QDir::separator());
            parentNode = createOrFindFolder(relativeNameList);
        }

        ProjectExplorer::FileNode *fNode = new ProjectExplorer::FileNode(file, ProjectExplorer::UnknownFileType, false);
        parentNode->addFileNodes({fNode});
    }
}

ProjectExplorer::FolderNode *SnapcraftGenericPartNode::createOrFindFolder(const QStringList &folder)
{
    QStringList watches;
    ProjectExplorer::FolderNode *currFolder = this;

    Utils::FileName currentPath = filePath();

    for (const QString &folderName: folder) {

        QList<ProjectExplorer::FolderNode *> subnodes = currFolder->subFolderNodes();
        currentPath = currentPath.appendPath(folderName);

        auto check = [&folderName](ProjectExplorer::FolderNode *f) {
            return f->filePath().fileName() == folderName;
        };

        auto it = std::find_if(subnodes.begin(), subnodes.end(), check);

        if (it != subnodes.end()) {
            //node exists already
            currFolder = *it;
            continue;
        }

        //does not exist lets create a new one
        ProjectExplorer::FolderNode *fNode = new SnapcraftGenericPartFolderNode(currentPath, ProjectExplorer::FolderNodeType, folderName);
        currFolder->addFolderNodes({fNode});
        currFolder = fNode;

        watches << currentPath.toFileInfo().absoluteFilePath();
    }

    if (!watches.isEmpty())
        m_watcher->addPaths(watches);

    return currFolder;
}

Utils::FileNameList FindFilesAndDirsVisitor::filePaths() const
{
    return m_filePaths;
}

Utils::FileNameList FindFilesAndDirsVisitor::dirPaths() const
{
    return m_dirPaths;
}

void FindFilesAndDirsVisitor::visitProjectNode(ProjectExplorer::ProjectNode *projectNode)
{
    visitFolderNode(projectNode);
}

void FindFilesAndDirsVisitor::visitFolderNode(ProjectExplorer::FolderNode *folderNode)
{
    if (folderNode->nodeType() != ProjectExplorer::ProjectNodeType)
        m_dirPaths.append(folderNode->filePath());

    for (const ProjectExplorer::FileNode *fileNode : folderNode->fileNodes())
        m_filePaths.append(fileNode->filePath());
}

FindFileNodesForFileVisitor::FindFileNodesForFileVisitor(const Utils::FileName &f)
    : m_file(f)
{

}

QList<ProjectExplorer::FileNode *> FindFileNodesForFileVisitor::nodes() const
{
    return m_nodes;
}

void FindFileNodesForFileVisitor::visitProjectNode(ProjectExplorer::ProjectNode *projectNode)
{
    visitFolderNode(projectNode);
}

void FindFileNodesForFileVisitor::visitFolderNode(ProjectExplorer::FolderNode *folderNode)
{
    for(auto fileNode: folderNode->fileNodes()) {
        if (fileNode->filePath() == m_file)
            m_nodes.append(fileNode);
    }
}

FindNodesForFolderVisitor::FindNodesForFolderVisitor(const Utils::FileName &f)
    : m_folder(f)
{

}

QList<ProjectExplorer::FolderNode *> FindNodesForFolderVisitor::nodes() const
{
    return m_nodes;
}

void FindNodesForFolderVisitor::visitProjectNode(ProjectExplorer::ProjectNode *)
{
    return;
}

void FindNodesForFolderVisitor::visitFolderNode(ProjectExplorer::FolderNode *folderNode)
{
    if (m_folder == folderNode->filePath())
        m_nodes.append(folderNode);
}

bool SnapcraftGenericPartFolderNode::addFiles(const QStringList &, QStringList *)
{
    return true;
}

bool SnapcraftGenericPartFolderNode::removeFiles(const QStringList &, QStringList *)
{
    return true;
}

bool SnapcraftGenericPartFolderNode::deleteFiles(const QStringList &)
{
    return true;
}



} // namespace Internal
} // namespace Ubuntu
