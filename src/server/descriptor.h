/*******************************************************************************

   Copyright (C) 2007 M.K.A. <wyrmchild@users.sourceforge.net>

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:
   
   Except as contained in this notice, the name(s) of the above copyright
   holders shall not be used in advertising or otherwise to promote the sale,
   use or other dealings in this Software without prior written authorization.
   
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*******************************************************************************/

#ifndef DescriptorTemplate_INCLUDED
#define DescriptorTemplate_INCLUDED

#include "ref_counted.h"
#include "errors.h"

#include <unistd.h> // ::close()
#ifdef WIN32
	#include <winsock2.h>
	#include "socket.errors.h"
#else
	#include <fcntl.h> // ::fcntl()
#endif
#include <cassert> // assert()

//! Generic file descriptor class
/**
 * @bug Implementations of this class need to check for unique() in dtor to end the life
 * of the handle properly. Currently there's no callback for last destruction provided by
 * ReferenceCounted class.
 */
template <typename T>
class Descriptor
	: public ReferenceCounted
{
protected:
	//! Handle or file descriptor
	T m_handle;
	
	//! Last error
	int m_error;
public:
	//! Default ctor
	Descriptor();
	//! ctor
	Descriptor(T handle);
	//! copy ctor
	Descriptor(const Descriptor<T>& handle);
	//! abstract dtor
	virtual ~Descriptor();
	
	//! Get handle
	T handle() const __attribute__ ((nothrow));
	//! Set handle
	void setHandle(T handle) __attribute__ ((nothrow));
	//! Close handle
	virtual void close() __attribute__ ((nothrow));
	
	//! Write to descriptor
	virtual int write(char* buf, size_t len) __attribute__ ((nothrow,warn_unused_result));
	//! Read from descriptor
	virtual int read(char* buf, size_t len) __attribute__ ((nothrow,warn_unused_result));
	
	virtual bool block(bool x) __attribute__ ((nothrow));
	
	//! Return last error number
	int getError();
	
	#ifdef DESCRIPTOR_OPS
	Descriptor<T>& operator== (const Descriptor<T>& desc) __attribute__ ((nothrow));
	#endif
};

template <typename T>
Descriptor<T>::Descriptor()
	: ReferenceCounted(),
	m_handle(error::InvalidDescriptor)
{
}

template <typename T>
Descriptor<T>::Descriptor(T handle)
	: ReferenceCounted(),
	m_handle(handle)
{
}

template <typename T>
Descriptor<T>::Descriptor(const Descriptor<T>& d)
	: ReferenceCounted(d),
	m_handle(d.m_handle)
{
}

/** @bug Does not call inherited close() */
template <typename T>
Descriptor<T>::~Descriptor()
{
	if (unique())
		close();
}

template <typename T>
T Descriptor<T>::handle() const
{
	return m_handle;
}

template <typename T>
void Descriptor<T>::setHandle(T handle)
{
	close();
	m_handle = handle;
}

/**
 * @bug If we specialize this for SOCKET, GCC spews out piles of multiple definition errors.
 */
template <typename T>
void Descriptor<T>::close()
{
	#ifdef WIN32
	::closesocket(m_handle);
	m_handle = socket_error::InvalidHandle;
	#else
	::close(m_handle);
	m_handle = error::InvalidDescriptor;
	#endif
}

#ifdef WIN32
/*
template <>
void Descriptor<SOCKET>::close()
{
	std::cout << "SOCKET close()" << std::endl;
	::closesocket(m_handle);
	
	m_handle = socket_error::InvalidHandle;
}
*/
#endif


template <typename T>
int Descriptor<T>::write(char* buf, size_t len)
{
	assert(buf != 0);
	assert(len > 0);
	assert(m_handle != error::InvalidDescriptor);
	
	const int r = ::write(m_handle, buf, len);
	
	if (r == -1)
	{
		m_error = errno;
		
		assert(r != EBADF);
		assert(r != EFAULT);
		assert(r != EINVAL);
		
		//if (r == Interrupted or r == WouldBlock); //nonfatal
	}
	
	return r;
}

template <typename T>
int Descriptor<T>::read(char* buf, size_t len)
{
	assert(buf != 0);
	assert(len > 0);
	assert(m_handle != error::InvalidDescriptor);
	
	const int r = ::read(m_handle, buf, len);
	
	if (r == -1)
	{
		m_error = errno;
		
		assert(r != EBADF);
		assert(r != EFAULT);
		assert(r != EINVAL);
		
		//if (r == Interrupted or r == WouldBlock); //nonfatal
	}
	
	return r;
}

template <typename T>
bool Descriptor<T>::block(bool x)
{
	#ifdef WIN32
	return false;
	#else
	assert(x == false);
	return fcntl(m_sock, F_SETFL, O_NONBLOCK) == Error ? false : true;
	#endif
}

template <typename T>
int Descriptor<T>::getError()
{
	return m_error;
}

#ifdef DESCRIPTOR_OPS
template <typename T>
bool Descriptor<T>::operator== (const Descriptor<T>& desc) const
{
	return (m_handle == desc.m_handle);
}
#endif


#endif // DescriptorTemplate_INCLUDED