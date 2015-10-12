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

#pragma once

#include <launch/LaunchStep.h>
#include <launch/LoggedProcess.h>

class LaunchMinecraft: public LaunchStep
{
	Q_OBJECT
public:
	explicit LaunchMinecraft(LaunchTask *parent);
	virtual void executeTask();
	virtual bool abort();
	virtual void proceed();
	virtual bool canAbort() const
	{
		return true;
	}
	void setWorkingDirectory(const QString &wd);

	void setClasspath(const QStringList & classpath)
	{
		m_classpath = classpath;
	}
	void setMainclass(const QString & mainclass)
	{
		m_mainclass = mainclass;
	}
	void setParams(const QStringList & params)
	{
		m_params = params;
	}
	void setNativePath(const QString & nativePath)
	{
		m_nativePath = nativePath;
	}
private slots:
	void on_state(LoggedProcess::State state);

private:
	LoggedProcess m_process;

	QStringList m_classpath;
	QString m_mainclass;
	QStringList m_params;
	QString m_nativePath;

	bool mayProceed = false;
};
