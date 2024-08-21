#pragma once

#include <QStringList>

/**
 * @brief A helper class to create an argv vector for use in functions that require one.
 *
 * It can be difficult and cumbersome to create a new argv vector.
 * The programmer needs to do two layers of old-school dynamic memory allocations
 * and make sure to responsibly free them. This class abstracts away the allocation,
 * and automatically frees the memory when the object goes out of scope.
 *
 * DO NOT CALL add(...) or remove(...) after calling argv() unless you are done with it,
 * as the previously returned argv will be freed.
 */
class ArgvBuilder
{
public:
	ArgvBuilder();
	ArgvBuilder(char** argv, int argc);
	~ArgvBuilder();

	/**
	 * @brief add an argument to the end of the builder. Frees any argv that has already been allocated.
	 * @param arg the argument to add.
	 */
	void add(const QString &arg);

	/**
	 * @brief remove an argument and an optional number of parameters from the builder.
	 * Frees any argv that has been already allocated.
	 * @param arg The command line argument to seek.
	 * @param num The number of arguments to remove after arg.
	 * @return true if the argument was found, false otherwise.
	 */
	bool remove(const QString &arg, int num = 0);

	/**
	 * @return The resulting argv from the builder.
	 */
	char** argv();

	/**
	 * @return The number of arguments.
	 */
	int argc();
private:
	/**
	 * @brief frees the current argv allocation if applicable.
	 */
	void _free();

	QStringList _list;
	char** _argv;
};
