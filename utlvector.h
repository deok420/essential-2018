#pragma once
/*
template< class T, class I = int >
class CUtlMemory {
public:
	__forceinline T& operator[ ]( I i ) {
		return m_pMemory[ i ];
	}


	static int CalcNewAllocationCount( int count, int size, int requested, int bytes ) {
		if( size )
			count = ( ( 1 + ( ( requested - 1 ) / size ) ) * size );

		else {
			if( !count )
				count = ( 31 + bytes ) / bytes;

			while( count < requested )
				count *= 2;
		}

		return count;
	}

	__forceinline void Grow( int count = 1 ) {
		if( IsExternallyAllocated( ) )
			return;

		int requested = m_nAllocationCount + count;
		int new_alloc_count = CalcNewAllocationCount( m_nAllocationCount, m_nGrowSize, requested, sizeof( T ) );

		if( ( int )( I )new_alloc_count < requested ) {

			if( ( int )( I )new_alloc_count == 0 && ( int )( I )( new_alloc_count - 1 ) >= requested )
				--new_alloc_count;

			else {
				if( ( int )( I )requested != requested )
					return;

				while( ( int )( I )new_alloc_count < requested )
					new_alloc_count = ( new_alloc_count + requested ) / 2;
			}
		}

		m_nAllocationCount = new_alloc_count;

		if( m_pMemory )
			m_pMemory = ( T* )g_pMemAlloc->Realloc( m_pMemory, m_nAllocationCount * sizeof( T ) );

		else
			m_pMemory = ( T* )g_pMemAlloc->Alloc( m_nAllocationCount * sizeof( T ) );
	}

	__forceinline bool IsExternallyAllocated( ) const {
		return m_nGrowSize < 0;
	}

protected:
	T*  m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
};

template < class T >
__forceinline T* Construct( T* pMemory ) {
	return ::new( pMemory ) T;
}

template <class T>
__forceinline void Destruct( T* pMemory ) {
	pMemory->~T( );
}

template< class T, class A = CUtlMemory< T > >
class CUtlVector {
	typedef A CAllocator;
public:
	__forceinline T& operator[]( int i ) {
		return m_Memory[ i ];
	}

	__forceinline T& Element( int i ) {
		return m_Memory[ i ];
	}

	__forceinline T* Base( ) {
		return m_Memory.Base( );
	}

	__forceinline int Count( ) const {
		return m_Size;
	}

	__forceinline void RemoveAll( ) {
		for( int i = m_Size; --i >= 0; )
			Destruct( &Element( i ) );

		m_Size = 0;
	}

	__forceinline int AddToTail( ) {
		return InsertBefore( m_Size );
	}

	__forceinline int InsertBefore( int elem ) {
		GrowVector( );
		ShiftElementsRight( elem );
		Construct( &Element( elem ) );

		return elem;
	}

protected:
	__forceinline void GrowVector( int num = 1 ) {
		if( m_Size + num > m_Memory.NumAllocated( ) )
			m_Memory.Grow( m_Size + num - m_Memory.NumAllocated( ) );

		m_Size += num;
		ResetDbgInfo( );
	}

	__forceinline void ShiftElementsRight( int elem, int num = 1 ) {
		int numToMove = m_Size - elem - num;
		if( ( numToMove > 0 ) && ( num > 0 ) )
			memmove( &Element( elem + num ), &Element( elem ), numToMove * sizeof( T ) );
	}

	CAllocator m_Memory;
	int        m_Size;
	T*         m_pElements;

	__forceinline void ResetDbgInfo( ) {
		m_pElements = Base( );
	}
};*/
#pragma once
#include <cassert>

#include "utlmemory.h"

template < typename _value_t >
struct utl_mem_t {
	_value_t* m_ptr{};
	int			m_alloc_count{};
	int			m_grow_size{};
};

template < typename _value_t >
struct utl_vec_t {
	__forceinline _value_t& at(const int i);

	__forceinline const _value_t& at(const int i) const;

	utl_mem_t< _value_t >	m_mem{};
	int						m_size{};
	_value_t* m_elements{};
};

template< class T, class A = CUtlMemory< T > >
class CUtlVector {
	typedef A CAllocator;

	typedef T* iterator;
	typedef const T* const_iterator;
public:
	T& operator[](int i);
	const T& operator[](int i) const;

	T& Element(int i)
	{
		return m_Memory[i];
	}

	const T& Element(int i) const
	{
		//assert( IsValidIndex( i ) );
		return m_Memory[i];
	}

	T* Base()
	{
		return m_Memory.Base();
	}

	int Count() const
	{
		return m_Size;
	}

	void RemoveAll()
	{
		for (int i = m_Size; --i >= 0; )
			Destruct(&Element(i));

		m_Size = 0;
	}

	bool IsValidIndex(int i) const
	{
		return (i >= 0) && (i < m_Size);
	}

	void ShiftElementsRight(int elem, int num = 1)
	{
		//assert( IsValidIndex( elem ) || ( m_Size == 0 ) || ( num == 0 ) );
		int numToMove = m_Size - elem - num;
		if ((numToMove > 0) && (num > 0))
		{
			memmove(&Element(elem + num), &Element(elem), numToMove * sizeof(T));
		}
	}

	void ShiftElementsLeft(int elem, int num = 1)
	{
		//assert( IsValidIndex( elem ) || ( m_Size == 0 ) || ( num == 0 ) );
		int numToMove = m_Size - elem - num;
		if ((numToMove > 0) && (num > 0))
		{
			memmove(&Element(elem), &Element(elem + num), numToMove * sizeof(T));
		}
	}

	void GrowVector(int num = 1)
	{
		if (m_Size + num > m_Memory.NumAllocated())
		{
			m_Memory.Grow(m_Size + num - m_Memory.NumAllocated());
		}

		m_Size += num;
	}

	int InsertBefore(int elem)
	{
		// Can insert at the end
		//assert( ( elem == Count( ) ) || IsValidIndex( elem ) );

		GrowVector();
		ShiftElementsRight(elem);
		Construct(&Element(elem));
		return elem;
	}

	int AddToHead()
	{
		return InsertBefore(0);
	}

	int AddToTail() {
		return InsertBefore(m_Size);
	}

	int InsertBefore(int elem, const T& src)
	{
		// Can't insert something that's in the list... reallocation may hose us
		assert((Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count())));

		// Can insert at the end
		//assert( ( elem == Count( ) ) || IsValidIndex( elem ) );

		GrowVector();
		ShiftElementsRight(elem);
		CopyConstruct(&Element(elem), src);
		return elem;
	}

	int AddToTail(const T& src)
	{
		// Can't insert something that's in the list... reallocation may hose us
		return InsertBefore(m_Size, src);
	}

	int Find(const T& src) const
	{
		for (int i = 0; i < Count(); ++i)
		{
			if (Element(i) == src)
			{
				return i;
			}
		}
		return -1;
	}

	void Remove(int elem)
	{
		Destruct(&Element(elem));
		ShiftElementsLeft(elem);
		--m_Size;
	}

	bool FindAndRemove(const T& src)
	{
		int elem = Find(src);
		if (elem != -1)
		{
			Remove(elem);
			return true;
		}
		return false;
	}

	iterator begin()
	{
		return Base();
	}

	const_iterator begin() const
	{
		return Base();
	}

	iterator end()
	{
		return Base() + Count();
	}

	const_iterator end() const
	{
		return Base() + Count();
	}

	//protected:
	CAllocator m_Memory;
	int m_Size;
	T* m_pElements;
};

template< typename T, class A >
T& CUtlVector< T, A >::operator[](int i)
{
	assert(i < m_Size);
	return m_Memory[i];
}

template< typename T, class A >
const T& CUtlVector< T, A >::operator[](int i) const
{
	assert(i < m_Size);
	return m_Memory[i];
}
