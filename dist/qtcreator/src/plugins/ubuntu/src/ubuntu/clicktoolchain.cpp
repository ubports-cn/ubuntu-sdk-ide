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

#include "clicktoolchain.h"
#include "ubuntuconstants.h"

#include <utils/fileutils.h>
#include <utils/algorithm.h>
#include <projectexplorer/abi.h>
#include <QDebug>
#include <QVariant>
#include <QVariantMap>
#include <QFile>
#include <QFileInfo>

namespace Ubuntu {
namespace Internal {

enum {
    debug = 0
};

QLatin1String UBUNTU_TARGET_ARCH_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.Arch");
QLatin1String UBUNTU_TARGET_FRAMEWORK_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.Framework");
QLatin1String UBUNTU_TARGET_CONTAINER_KEY = QLatin1String("Ubuntu.ClickToolChain.Target.ContainerName");

const char UBUNTU_CLICK_GCC_WRAPPER[]    = "qtc_chroot_gcc";    //deprecated just for updating

#if 1
static QMap <QString,ProjectExplorer::Abi> init_architectures()
{
    QMap<QString,ProjectExplorer::Abi> map;
    map.insert(QLatin1String("armhf") , ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture,
                                                             ProjectExplorer::Abi::LinuxOS,
                                                             ProjectExplorer::Abi::GenericLinuxFlavor,
                                                             ProjectExplorer::Abi::ElfFormat,
                                                             32));

    map.insert(QLatin1String("i386") ,ProjectExplorer::Abi(ProjectExplorer::Abi::X86Architecture,
                                                           ProjectExplorer::Abi::LinuxOS,
                                                           ProjectExplorer::Abi::GenericLinuxFlavor,
                                                           ProjectExplorer::Abi::ElfFormat,
                                                           32));

    map.insert(QLatin1String("amd64"),ProjectExplorer::Abi(ProjectExplorer::Abi::X86Architecture,
                                                           ProjectExplorer::Abi::LinuxOS,
                                                           ProjectExplorer::Abi::GenericLinuxFlavor,
                                                           ProjectExplorer::Abi::ElfFormat,
                                                           64));
    map.insert(QLatin1String("arm64") , ProjectExplorer::Abi(ProjectExplorer::Abi::ArmArchitecture,
                                                             ProjectExplorer::Abi::LinuxOS,
                                                             ProjectExplorer::Abi::GenericLinuxFlavor,
                                                             ProjectExplorer::Abi::ElfFormat,
                                                             64));
    return map;
}

QMap <QString,ProjectExplorer::Abi> clickArchitectures = init_architectures();
#endif


QList<Utils::FileName> ClickToolChain::suggestedMkspecList() const
{
    return ProjectExplorer::GccToolChain::suggestedMkspecList();
    //return QList<Utils::FileName>()<< Utils::FileName::fromString(QLatin1String("linux-g++"));
}

Utils::FileName ClickToolChain::suggestedDebugger() const
{
    return Utils::FileName::fromString(QLatin1String("/usr/bin/gdb-multiarch"));
}

QString ClickToolChain::typeDisplayName() const
{
    return ClickToolChainFactory::tr("Ubuntu GCC");
}

bool ClickToolChain::isValid() const
{
    return GccToolChain::isValid() && targetAbi().isValid() && UbuntuClickTool::targetExists(m_clickTarget);
}

void ClickToolChain::addToEnvironment(Utils::Environment &env) const
{
    GccToolChain::addToEnvironment(env);
    env.set(QLatin1String("CLICK_SDK_ARCH")     , m_clickTarget.architecture);
    env.set(QLatin1String("CLICK_SDK_FRAMEWORK"), m_clickTarget.framework);
}

QString ClickToolChain::makeCommand(const Utils::Environment &) const
{
    return UbuntuClickTool::findOrCreateMakeWrapper(clickTarget());
}

bool ClickToolChain::operator ==(const ProjectExplorer::ToolChain &tc) const
{
    if (!GccToolChain::operator ==(tc))
        return false;

    return (m_clickTarget.architecture == m_clickTarget.architecture
            && m_clickTarget.framework == m_clickTarget.framework
            && m_clickTarget.containerName == m_clickTarget.containerName);
}

ProjectExplorer::ToolChainConfigWidget *ClickToolChain::configurationWidget()
{
    return GccToolChain::configurationWidget();
}

const UbuntuClickTool::Target &ClickToolChain::clickTarget() const
{
    return m_clickTarget;
}

ProjectExplorer::Abi ClickToolChain::architectureNameToAbi(const QString &arch)
{
    if( !clickArchitectures.contains(arch) )
        return ProjectExplorer::Abi();

    return clickArchitectures[arch];
}

QString ClickToolChain::abiToArchitectureName(const ProjectExplorer::Abi &abi)
{
    for (auto i = clickArchitectures.constBegin(); i != clickArchitectures.constEnd(); i++){
        if (i.value() == abi)
            return i.key();
    }
    return QString("unknown");
}

QList<QString> ClickToolChain::supportedArchitectures()
{
    return clickArchitectures.keys();
}

QString ClickToolChain::remoteCompilerCommand() const
{
    return QString::fromLatin1("/usr/bin/%1").arg(compilerCommand().fileName());
}

QVariantMap ClickToolChain::toMap() const
{
    QVariantMap map = GccToolChain::toMap();
    map.insert(UBUNTU_TARGET_ARCH_KEY,m_clickTarget.architecture);
    map.insert(UBUNTU_TARGET_FRAMEWORK_KEY,m_clickTarget.framework);
    map.insert(UBUNTU_TARGET_CONTAINER_KEY,m_clickTarget.containerName);
    return map;
}

QString ClickToolChain::gnutriplet() const
{
    return gnutriplet(targetAbi());
}

QString ClickToolChain::gnutriplet(const ProjectExplorer::Abi &abi)
{
    switch(abi.architecture()) {
        case ProjectExplorer::Abi::ArmArchitecture:
            switch(abi.wordWidth())
            {
                case 32:
                    return QLatin1String("arm-linux-gnueabihf");
                case 64:
                    return QLatin1String("aarch64-linux-gnu");
            }
            break;
        case ProjectExplorer::Abi::X86Architecture:
            switch(abi.wordWidth())
            {
                case 32:
                    return QLatin1String("i386-linux-gnu");
                case 64:
                    return QLatin1String("x86_64-linux-gnu");
            }
            break;
        default:
            Q_ASSERT_X(false,Q_FUNC_INFO,"Unknown Target architecture");
    }
    return QString();
}

bool ClickToolChain::fromMap(const QVariantMap &data)
{
    if(!GccToolChain::fromMap(data))
        return false;

    if(!data.contains(UBUNTU_TARGET_ARCH_KEY)
            || !data.contains(UBUNTU_TARGET_FRAMEWORK_KEY)
            || !data.contains(UBUNTU_TARGET_CONTAINER_KEY))
        return false;

    m_clickTarget.architecture  = data[UBUNTU_TARGET_ARCH_KEY].toString();
    m_clickTarget.framework     = data[UBUNTU_TARGET_FRAMEWORK_KEY].toString();
    m_clickTarget.containerName = data[UBUNTU_TARGET_CONTAINER_KEY].toString();
    return isValid();
}

Utils::FileName ClickToolChain::compilerCommand() const
{
    return GccToolChain::compilerCommand();
}

ClickToolChain::ClickToolChain(const UbuntuClickTool::Target &target, Detection d)
    : GccToolChain(Constants::UBUNTU_CLICK_TOOLCHAIN_ID, d)
    , m_clickTarget(target)
{
    resetToolChain(Utils::FileName::fromString(
                           UbuntuClickTool::findOrCreateGccWrapper(target)
                           ));

    setDisplayName(QString::fromLatin1("Ubuntu GCC (%1-%2-%3)")
                   .arg(ProjectExplorer::Abi::toString(targetAbi().architecture()))
                   .arg(target.framework)
                   .arg(target.containerName));
}

ClickToolChain::ClickToolChain(const ClickToolChain &other)
    : GccToolChain(other)
    , m_clickTarget(other.m_clickTarget)
{
}

ClickToolChain::ClickToolChain()
    : GccToolChain(Constants::UBUNTU_CLICK_TOOLCHAIN_ID,ManualDetection)
{
}

ClickToolChainFactory::ClickToolChainFactory()
{
    setDisplayName(tr("Ubuntu GCC"));
}

QList<ProjectExplorer::ToolChain *> ClickToolChainFactory::autoDetect(
        const QList<ProjectExplorer::ToolChain *> &alreadyKnown)
{
    return createToolChainsForClickTargets(alreadyKnown);
}

bool ClickToolChainFactory::canRestore(const QVariantMap &data)
{
    return typeIdFromMap(data) == Constants::UBUNTU_CLICK_TOOLCHAIN_ID;
}

ProjectExplorer::ToolChain *ClickToolChainFactory::restore(const QVariantMap &data)
{
    ClickToolChain *tc = new ClickToolChain();
    if (tc->fromMap(data)) {
        QFileInfo compilerCommand(tc->compilerCommand().toString());
        //deprecated wrapper script, update to use the new one
        if(compilerCommand.fileName() == QString::fromLatin1(UBUNTU_CLICK_GCC_WRAPPER)) {
            QString wrapper = UbuntuClickTool::findOrCreateGccWrapper(tc->clickTarget());

            //if we cannot create a good wrapper its better to drop the ToolChain
            if(wrapper.isNull()) {
                delete tc;
                return 0;
            }
            tc->setCompilerCommand(Utils::FileName::fromString(wrapper));
        }
        return tc;
    }

    delete tc;
    return 0;
}

QList<ProjectExplorer::ToolChain *> ClickToolChainFactory::createToolChainsForClickTargets(const QList<ProjectExplorer::ToolChain *> &alreadyKnown)
{
    QList<ProjectExplorer::ToolChain*> toolChains;

    QList<UbuntuClickTool::Target> targets = UbuntuClickTool::listAvailableTargets();
    foreach(const UbuntuClickTool::Target &target, targets) {
        if(debug) qDebug()<<"Found Target"<<target;

        if(!clickArchitectures.contains(target.architecture))
            continue;

        QString comp = UbuntuClickTool::findOrCreateGccWrapper(target);
        if(comp.isEmpty())
            continue;

        auto predicate = [&](ProjectExplorer::ToolChain *tc) {
            if (tc->typeId() != Constants::UBUNTU_CLICK_TOOLCHAIN_ID)
                return false;
            auto clickTc = static_cast<ClickToolChain *>(tc);
            return clickTc->clickTarget().containerName == target.containerName;
        };

        ProjectExplorer::ToolChain *tc = Utils::findOrDefault(alreadyKnown, predicate);
        if (!tc)
            tc = new ClickToolChain(target, ProjectExplorer::ToolChain::AutoDetection);
        toolChains.append(tc);
    }

    return toolChains;
}

} // namespace Internal
} // namespace Ubuntu
