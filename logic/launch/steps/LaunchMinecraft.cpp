/* Copyright 2013-2015 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LaunchMinecraft.h"
#include <launch/LaunchTask.h>
#include <minecraft/OneSixInstance.h>
#include <FileSystem.h>
#include <QStandardPaths>

LaunchMinecraft::LaunchMinecraft(LaunchTask *parent) : LaunchStep(parent)
{
	connect(&m_process, &LoggedProcess::log, this, &LaunchMinecraft::logLines);
	connect(&m_process, &LoggedProcess::stateChanged, this, &LaunchMinecraft::on_state);
}

void LaunchMinecraft::executeTask()
{
	auto instance = m_parent->instance();
	std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
	QStringList args = minecraftInstance->javaArguments();

	// HACK: this is a workaround for MCL-3732 - 'server-resource-packs' is created.
	if(!FS::ensureFolderPathExists(FS::PathCombine(minecraftInstance->minecraftRoot(), "server-resource-packs")))
	{
		emit logLine(tr("Couldn't create the 'server-resource-packs' folder"), MessageLevel::Error);
	}

	QString allArgs = args.join(", ");
	emit logLine("Java Arguments:\n  [" + m_parent->censorPrivateInfo(allArgs) + "]\n\n", MessageLevel::MultiMC);

	if(m_classpath.size())
	{
		emit logLine("Libraries:", MessageLevel::MultiMC);
		for(auto & lib: m_classpath)
		{
			QFileInfo f(lib);
			QString niceLib = FS::NormalizePath(lib);
			if (f.exists())
			{
				emit logLine(QString("  %1").arg(niceLib), MessageLevel::MultiMC);
			}
			else
			{
				emit logLine(QString("  %1 (missing)").arg(niceLib), MessageLevel::Warning);
			}
		}
		args.push_back("-cp");
#ifdef Q_OS_WIN
		args.push_back(m_classpath.join(";"));
#else
		args.push_back(m_classpath.join(":"));
#endif
	}
	emit logLine(QString(), MessageLevel::MultiMC);

	emit logLine(QString("Native path:\n  %1\n\n").arg(m_nativePath), MessageLevel::MultiMC);
	args.push_back(QString("-Djava.library.path=%1").arg(m_nativePath));

	emit logLine(QString("Main class:\n   %1\n\n").arg(m_mainclass), MessageLevel::MultiMC);
	args.push_back(m_mainclass);

	QString allParams = m_params.join(", ");
	emit logLine("Params:\n  [" + m_parent->censorPrivateInfo(allParams) + "]\n\n", MessageLevel::MultiMC);
	args.append(m_params);

	auto javaPath = FS::ResolveExecutable(instance->settings()->get("JavaPath").toString());

	m_process.setProcessEnvironment(instance->createEnvironment());

	QString wrapperCommand = instance->getWrapperCommand();
	if(!wrapperCommand.isEmpty())
	{
		auto realWrapperCommand = QStandardPaths::findExecutable(wrapperCommand);
		if (realWrapperCommand.isEmpty())
		{
			QString reason = tr("The wrapper command \"%1\" couldn't be found.").arg(wrapperCommand);
			emit logLine(reason, MessageLevel::Fatal);
			emitFailed(reason);
			return;
		}
		emit logLine("Wrapper command is:\n  " + wrapperCommand + "\n\n", MessageLevel::MultiMC);
		args.prepend(javaPath);
		m_process.start(wrapperCommand, args);
	}
	else
	{
		m_process.start(javaPath, args);
	}
}

void LaunchMinecraft::on_state(LoggedProcess::State state)
{
	switch(state)
	{
		case LoggedProcess::FailedToStart:
		{
			//: Error message displayed if instace can't start
			QString reason = tr("Could not launch minecraft!");
			emit logLine(reason, MessageLevel::Fatal);
			emitFailed(reason);
			return;
		}
		case LoggedProcess::Aborted:
		case LoggedProcess::Crashed:

		{
			m_parent->setPid(-1);
			emitFailed("Game crashed.");
			return;
		}
		case LoggedProcess::Finished:
		{
			m_parent->setPid(-1);
			//FIXME: make this work again
			// auto exitCode = m_process.exitCode();
			// m_postlaunchprocess.processEnvironment().insert("INST_EXITCODE", QString(exitCode));
			// run post-exit
			emitSucceeded();
			break;
		}
		case LoggedProcess::Running:
			emit logLine(tr("Minecraft process ID: %1\n\n").arg(m_process.processId()), MessageLevel::MultiMC);
			m_parent->setPid(m_process.processId());
			m_parent->instance()->setLastLaunch();
			mayProceed = true;
			emit readyForLaunch();
			break;
		default:
			break;
	}
}

void LaunchMinecraft::setWorkingDirectory(const QString &wd)
{
	m_process.setWorkingDirectory(wd);
}

void LaunchMinecraft::proceed()
{
	if(mayProceed)
	{
		QString launchString("launch\n");
		m_process.write(launchString.toUtf8());
		mayProceed = false;
	}
}

bool LaunchMinecraft::abort()
{
	if(mayProceed)
	{
		mayProceed = false;
		QString launchString("abort\n");
		m_process.write(launchString.toUtf8());
	}
	else
	{
		auto state = m_process.state();
		if (state == LoggedProcess::Running || state == LoggedProcess::Starting)
		{
			m_process.kill();
		}
	}
	return true;
}
