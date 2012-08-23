
/*
 * pty.cpp
 *
 * Pseudo terminal access class
 *
 *
 * 06.07.2010, Klaus Hennemann
 */

#include "pty.h"


Pty::Pty (QObject *parent)
	: QThread(parent)
{
	pty = Betty_CreatePty();
}


Pty::~Pty ()
{
	Betty_KillPtyChild(pty);
	wait(2000);
	Betty_DestroyPty(pty);
}


bool Pty::fork (const char *filename,
		char * const argv[],
		char * const envp[])
{
	if (isRunning())
		return false;

	if (Betty_ForkPty(pty, filename, argv, envp) < 0)
		return false;

	start();
	return true;
}


bool Pty::kill ()
{
	if (Betty_KillPtyChild(pty) < 0)
		return false;

	return true;
}


int Pty::write (const void *buf, unsigned int count)
{
	return Betty_WritePtyData(pty, buf, count);	
}


void Pty::run ()
{
	unsigned char buf[256];
	int count;

	while ( (count = Betty_ReadPtyData(pty, buf, sizeof(buf))) > 0 )
		emit dataReceived(QByteArray((char *)buf, count));

	kill();
}


