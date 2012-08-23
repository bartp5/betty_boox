
#ifndef __PTY_H__
#define __PTY_H__

#include <QThread>
#include <QByteArray>

#include <betty/pty.h>


class Pty : public QThread {
	Q_OBJECT

public:

	Pty (QObject *parent = 0);
	~Pty ();

	bool fork (const char *filename,
			char * const argv[], char * const envp[]);
	bool kill ();
	int write (const void *buf, unsigned int count);

signals:

	void dataReceived (QByteArray data);

protected:

	void run ();

private:

	struct Betty_Pty *pty;
};


#endif

