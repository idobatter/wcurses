#ifndef _pyhelper_h
#define _pyhelper_h

/*
	Some helper Macros for writing python callable functions in C.
*/


// How many values in an args?
#define ARG_COUNT(X) \
	(((X) == NULL) ? 0 : (PyTuple_Check(X) ? PyTuple_Size(X) : 1))

// Check the result of a Python function:
// If NULL was returned there was an exception, and we propagate it.
// Otherwise we decrement the refcount on the returned object (ignoring it.)
#define CHECK(result) \
	{if(!(result)) \
		return NULL; \
	else \
		Py_DECREF((result));}

PyObject *Noop(PyObject *self,PyObject *args);
PyObject *NoArg_None(PyObject *self);
PyObject *NoArg_False(PyObject *self);
PyObject *NoArg_True(PyObject *self);

#endif
