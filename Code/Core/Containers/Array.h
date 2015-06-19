// Array.h
//------------------------------------------------------------------------------
#pragma once
#ifndef CORE_CONTAINERS_ARRAY_H
#define CORE_CONTAINERS_ARRAY_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Sort.h"
#include "Core/Env/Assert.h"
#include "Core/Env/Types.h"
#include "Core/Math/Conversions.h"
#include "Core/Mem/Mem.h"

// Array
//------------------------------------------------------------------------------
template < class T >
class Array
{
public:
	explicit Array();
	explicit Array( const Array< T > & other );
	explicit Array( const T * begin, const T * end );
	explicit Array( size_t initialCapacity, bool resizeable = false );
	~Array();

	void Destruct();

	// iterators and access
	typedef	T *			Iter;
	typedef const T *	ConstIter;
	Iter		Begin()	const	{ return m_Begin; }
	Iter		End() const		{ return m_End; }
	inline T &			operator [] ( size_t index )		{ ASSERT( index < GetSize() ); return m_Begin[ index ]; }
	inline const T &	operator [] ( size_t index ) const	{ ASSERT( index < GetSize() ); return m_Begin[ index ]; }
	inline T &			Top()		{ ASSERT( m_Begin < m_End ); return m_End[ -1 ]; }
	inline const T &	Top() const	{ ASSERT( m_Begin < m_End ); return m_End[ -1 ]; }

    // C++11 style for range based for
	Iter		begin()	        { return m_Begin; }
	ConstIter	begin()	const   { return m_Begin; }
	Iter		end()           { return m_End; }
	ConstIter	end() const     { return m_End; }

	// modify capacity/size
	void SetCapacity( size_t capacity );
	void SetSize( size_t size );
	void Clear();
	void Swap( Array< T > & other );

	// sorting
	void Sort() { ShellSort( m_Begin, m_End, AscendingCompare() ); }
	void SortDeref() { ShellSort( m_Begin, m_End, AscendingCompareDeref() ); }
	template < class COMPARER >
	void Sort( const COMPARER & comp ) { ShellSort( m_Begin, m_End, comp ); }

	// find
	template < class U >
	T * Find( const U & obj ) const;
	template < class U >
	T * FindDeref( const U & obj ) const;

	// add/remove items
	void Append( const T & item );
	template < class U >
	void Append( const Array< U > & other );
	template < class U >
	void Append( const U * begin, const U * end );
	void Pop();
	void PopFront(); // expensive - shuffles everything in the array!
	void Erase( T * const iter );
	inline void EraseIndex( size_t index ) { Erase( m_Begin + index ); }

	Array & operator = ( const Array< T > & other );

	// query state
	inline bool		IsAtCapacity() const	{ return ( m_End == m_MaxEnd ) && ( m_Resizeable == false ); }
	inline size_t	GetCapacity() const		{ return ( m_MaxEnd - m_Begin ); }
	inline size_t	GetSize() const			{ return ( m_End - m_Begin ); }
	inline bool		IsEmpty() const			{ return ( m_Begin == m_End ); }

private:
	void Grow();
	inline T * Allocate( size_t numElements ) const;
	inline void Deallocate( T * ptr ) const;

	T * m_Begin;
	T * m_End;
	T * m_MaxEnd;
	bool m_Resizeable;
};

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array()
	: m_Begin( nullptr )
	, m_End( nullptr )
	, m_MaxEnd( nullptr )
	, m_Resizeable( true )
{
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( const Array< T > & other )
{
	INPLACE_NEW (this) Array( other.GetSize(), true );
	*this = other;
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( const T * begin, const T * end )
{
	const size_t size = ( end - begin );
	INPLACE_NEW (this) Array( size, true );
	Append( begin, end );
}

// CONSTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::Array( size_t initialCapacity, bool resizeable )
{
	if ( initialCapacity )
	{
		#ifdef ASSERTS_ENABLED
			m_Resizeable = true; // allow initial allocation
		#endif
		m_Begin = Allocate( initialCapacity );
		m_End = m_Begin;
		m_MaxEnd = m_Begin + initialCapacity;
	}
	else
	{
		m_Begin = nullptr;
		m_End = nullptr;
		m_MaxEnd = nullptr;
	}
	m_Resizeable = resizeable;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
template < class T >
Array< T >::~Array()
{
	Destruct();
}

// Destruct
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Destruct()
{
	T * iter = m_Begin;
	while ( iter < m_End )
	{
		iter->~T();
		iter++;
	}
	Deallocate( m_Begin );
	m_Begin = nullptr;
	m_End = nullptr;
	m_MaxEnd = nullptr;
}

// SetCapacity
//------------------------------------------------------------------------------
template < class T >
void Array< T >::SetCapacity( size_t capacity )
{
	if ( capacity == GetCapacity() )
	{
		return;
	}

	T * newMem = Allocate( capacity );

	// transfer and items across that can fit, and
	// destroy all the originals
	size_t itemsToKeep = Math::Min( capacity, GetSize() );
	T * src = m_Begin;
	T * dst = newMem;
	T * keepEnd = m_Begin + itemsToKeep;
	while ( src < m_End )
	{
		if ( src < keepEnd )
		{
			INPLACE_NEW ( dst ) T( *src );
		}
		src->~T();
		src++;
		dst++;
	}

	// free old memory
	Deallocate( m_Begin );

	// hook up to new memory
	m_Begin = newMem;
	m_End = newMem + itemsToKeep;
	m_MaxEnd = newMem + capacity;
}

// SetSize
//------------------------------------------------------------------------------
template < class T >
void Array< T >::SetSize( size_t size )
{
	size_t oldSize = GetSize();

	// no change
	if ( oldSize == size )
	{
		return;
	}

	// shrink
	if ( size < oldSize )
	{
		// destroy excess items
		T * item = m_Begin + size;
		T * end = m_End;
		while ( item < end )
		{
			item->~T();
			item++;
		}
		m_End = m_Begin + size;
		return;
	}

	// grow

	// ensure there is enough capacity
	if ( size > GetCapacity() )
	{
		SetCapacity( size );
	}

	// create additional new items
	T * item = m_End;
	T * newEnd = m_Begin + size;
	while( item < newEnd )
	{
		INPLACE_NEW ( item ) T;
		item++;
	}
	m_End = newEnd;
}

// Clear
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Clear()
{
	// destroy all items
	T * src = m_Begin;
	while ( src < m_End )
	{
		src->~T();
		src++;
	}

	// set to empty, but do not free memory
	m_End = m_Begin;
}

// Find
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Swap( Array< T > & other )
{
	T * tmpBegin = m_Begin;
	T * tmpEnd = m_End;
	T * tmpMaxEnd = m_MaxEnd;
	bool tmpResizeable = m_Resizeable;
	m_Begin = other.m_Begin;
	m_End = other.m_End;
	m_MaxEnd = other.m_MaxEnd;
	m_Resizeable = other.m_Resizeable;
	other.m_Begin = tmpBegin;
	other.m_End = tmpEnd;
	other.m_MaxEnd = tmpMaxEnd;
	other.m_Resizeable = tmpResizeable;
}

// Find
//------------------------------------------------------------------------------
template < class T >
template < class U >
T * Array< T >::Find( const U & obj ) const
{
	T * pos = m_Begin;
	T * end = m_End;
	while ( pos < end )
	{
		if ( *pos == obj )
		{
			return pos;
		}
		pos++;
	}
	return nullptr;
}

// FindDeref
//------------------------------------------------------------------------------
template < class T >
template < class U >
T * Array< T >::FindDeref( const U & obj ) const
{
	T * pos = m_Begin;
	T * end = m_End;
	while ( pos < end )
	{
		if ( *(*pos) == obj )
		{
			return pos;
		}
		pos++;
	}
	return nullptr;
}

// Append
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Append( const T & item )
{
	if ( m_End == m_MaxEnd )
	{
		Grow();
	}
	INPLACE_NEW ( m_End ) T( item );
	m_End++;
}

// Append
//------------------------------------------------------------------------------
template < class T >
template < class U >
void Array< T >::Append( const Array< U > & other  )
{
	U* end = other.End();
	for ( U* it = other.Begin(); it != end; ++it )
	{
		Append( *it );
	}	
}

// Append
//------------------------------------------------------------------------------
template < class T >
template < class U >
void Array< T >::Append( const U * begin, const U * end )
{
	for ( const U* it = begin; it != end; ++it )
	{
		Append( *it );
	}	
}

// Pop
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Pop()
{
	ASSERT( m_Begin < m_End ); // something must be in the array

	T * it = --m_End;
	it->~T();
	(void)it; // avoid warning for arrays of pod types (like uint32_t)
}

// PopFront
//------------------------------------------------------------------------------
template < class T >
void Array< T >::PopFront()
{
	ASSERT( m_Begin < m_End ); // something must be in the array

	// shuffle everything backwards 1 element, overwriting the top elem
	T * dst = m_Begin;
	T * src = m_Begin + 1;
	while ( src < m_End )
	{
		*dst = *src;
		dst++;
		src++;
	}

	// free last element (which is now a dupe)
	dst->~T();

	m_End--;
}

// Erase (iter)
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Erase( T * const iter )
{
	ASSERT( iter < m_End );

	T * dst = iter;
	T * last = ( m_End - 1 );
	while ( dst < last )
	{
		*dst = *(dst + 1);
		dst++;
	}
	dst->~T();
	m_End = last;
}

// Grow
//------------------------------------------------------------------------------
template < class T >
Array< T > & Array< T >::operator = ( const Array< T > & other )
{
	Clear();

	// need to reallocate?
	const size_t otherSize = other.GetSize();
	if ( GetCapacity() < otherSize )
	{
		Deallocate( m_Begin );
		m_Begin = Allocate( otherSize );
		m_MaxEnd = m_Begin + otherSize;
	}

	m_End = m_Begin + otherSize;
	T * dst = m_Begin;
	T * src = other.m_Begin;
	const T * end = m_End;
	while ( dst < end )
	{
		INPLACE_NEW ( dst ) T( *src );
		dst++;
		src++;
	}
	
	return *this;
}

// Grow
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Grow()
{
	ASSERT( m_Resizeable );

	// grow by 1.5 times (but at least by one)
	size_t currentCapacity = GetCapacity();
	size_t size = GetSize();
	size_t newCapacity = ( currentCapacity + ( currentCapacity >> 1 ) + 1 );
	T * newMem = Allocate( newCapacity );

	T * src = m_Begin;
	T * dst = newMem;
	while ( src < m_End )
	{
		INPLACE_NEW ( dst ) T( *src );
		src->~T();
		dst++;
		src++;
	}
	Deallocate( m_Begin );
	m_Begin = newMem;
	m_End = ( newMem ) + size;
	m_MaxEnd = ( newMem ) + newCapacity;
}

// Allocate
//------------------------------------------------------------------------------
template < class T >
T * Array< T >::Allocate( size_t numElements ) const
{
	ASSERT( m_Resizeable == true );
    const size_t align = __alignof( T ) > sizeof( void * ) ? __alignof( T ) : sizeof( void * );
	return static_cast< T * >( ALLOC( sizeof( T ) * numElements, align ) );
}

// Deallocate
//------------------------------------------------------------------------------
template < class T >
void Array< T >::Deallocate( T * ptr ) const
{
	FREE( ptr );
}

//------------------------------------------------------------------------------
#endif // CORE_CONTAINERS_ARRAY_H
