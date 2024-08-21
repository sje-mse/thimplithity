#include "argvbuilder.h"

ArgvBuilder::ArgvBuilder() : _list(), _argv(nullptr)
{
	// Empty
}

ArgvBuilder::ArgvBuilder(char** argv, int argc) : ArgvBuilder()
{
	for (int i = 0; i < argc; i++) {
		_list.append(argv[i]);
	}
}

ArgvBuilder::~ArgvBuilder()
{
	_free();
}

void ArgvBuilder::add(const QString &arg)
{
	_free();
	_list.append(arg);
}

bool ArgvBuilder::remove(const QString &arg, int num)
{
	_free();
	int i = _list.indexOf(arg);
	if (i == -1) {
		return false;
	}
	for (int j = i + num; j >= i; j--) {
		if (j < _list.size()) {
			_list.removeAt(j);
		}
	}
	return true;
}

char** ArgvBuilder::argv()
{
	if (_argv == nullptr) {
		_argv = new char*[_list.size()];
		for (int i = 0; i < _list.size(); i++) {
			const QByteArray ba = _list[i].toLocal8Bit();
			_argv[i] = new char[ba.size() + 1];
			qstrncpy(_argv[i], ba.data(), ba.size() + 1);
		}
	}
	return _argv;
}

int ArgvBuilder::argc()
{
	return _list.size();
}

void ArgvBuilder::_free()
{
	if (_argv != nullptr) {
		for (int i = 0; i < argc(); i++) {
			delete[] _argv[i];
		}
		delete[] _argv;
		_argv = nullptr;
	}
}
