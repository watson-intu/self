#include "self_android.h"

#include <jni.h>
#include <errno.h>

#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <android/log.h>
#include <boost/thread.hpp>

#include "utils/Log.h"
#include "utils/ThreadPool.h"

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "self_android", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "self_android", __VA_ARGS__))

int SelfMain(int argc, char ** argv );

static boost::thread * pSelfThread = NULL;
static std::vector<std::string> args;

class AndroidConsoleReactor : public ILogReactor
{
public:
	AndroidConsoleReactor(LogLevel a_MinLevel = LL_STATUS) : m_MinLevel(a_MinLevel)
	{}

	virtual void Process(const LogRecord & a_Record)
	{
		if (a_Record.m_Level >= m_MinLevel)
		{
			LOGI("[%s][%s][%s] %s\n",
				a_Record.m_Time.c_str(),
				Log::LevelText(a_Record.m_Level),
				a_Record.m_SubSystem.c_str(),
				a_Record.m_Message.c_str());
		}
	}
	virtual void SetLogLevel( LogLevel a_Level )
	{
		m_MinLevel = a_Level;
	}

private:
	LogLevel			m_MinLevel;
};

static void RunSelf()
{
	Log::RegisterReactor( new AndroidConsoleReactor() );

	char ** argv = new char *[ args.size() ];
	for(size_t i=0;i<args.size();++i)
		argv[i] = strdup( args[i].c_str() );

	SelfMain( args.size(), (char **)argv );

	for(size_t i=0;i<args.size();++i)
		free( argv[i] );
}


extern "C" {

jint Java_com_self_1instance_1android_SelfMain_Start(JNIEnv *env, jobject obj, jobjectArray stringArray )
{
	Log::Status( "JavaSelfMain", "SelfMain.Start() invoked." );
	if ( pSelfThread != NULL )
		return 1;

	args.clear();

	int count = env->GetArrayLength( stringArray );
	for(int i=0;i<count;++i)
	{
		jstring js = (jstring)(env->GetObjectArrayElement( stringArray, i ));
		args.push_back( env->GetStringUTFChars( js, 0 ) );
	}

	pSelfThread = new boost::thread( RunSelf );
	return 0;
}

jint Java_com_self_1instance_1android_SelfMain_IsRunning()
{
	return pSelfThread != 0;
}

jint Java_com_self_1instance_1android_SelfMain_Stop()
{
	Log::Status( "JavaSelfMain", "SelfMain.Stop() invoked." );
	if (pSelfThread != NULL)
	{
		ThreadPool::Instance()->StopMainThread();
		delete pSelfThread;

		return 0;
	}

	return 1;
}

}
