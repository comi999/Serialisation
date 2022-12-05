#pragma once
#include <Utils/FunctionTraits.hpp>

//==========================================================================
// Serialisation framework allows objects to be serialised to and from byte
// streams. Here is an example of making a class Serialiseable,
// Deserialiseable and Sizeable:
// struct ExampleStruct
// {
//     std::string        String;
//	   int                Int;
//     float              Float;
//     std::vector< int > Vector;
// 
// private:
// 
//     friend class Serialisation;
//     
//     void OnBeforeSerialise() {} // Optional
//     void OnAfterSerialise() {} // Optional
// 
//     void OnSerialise( Serialiser& a_Serialiser ) const
//     {
//         a_Serialiser << String << Int << Float << Vector;
//     }
// 
//     void OnBeforeDeserialise() {} // Optional
//     void OnAfterDeserialise() {} // Optional
// 
//     void OnDeserialise( Deserialiser& a_Deserialiser )
//     {
//         a_Deserialiser >> String >> Int >> Float >> Vector;
//     }
//     
//     void OnSize( Sizer& a_Sizer ) const
//     {
//         a_Sizer + String + Int + Float + Vector;
//     }
// }
// 
// int main()
// {
//     Sizer sizer;
// 
//     ExampleStruct example_in;
//     sizer + example_in;
//     
//     std::vector< byte_t > Buffer;
//     Buffer.resize( sizer );
//     
//     Serialiser serialiser( Buffer.data() );
//     serialiser << example_in;
//     
//	   ExampleStruct example_out;
//     Deserialiser deserialiser( Buffer.data() );
//     deserialiser >> example_out;
//     
//     assert( example_in == example_out );
// 
//     return 0;
// }
//==========================================================================

namespace std
{
template < class, class > class pair;
template < class... > class tuple;
template < class, class, class > class basic_string;
template < class, size_t > class array;
template < class, class > class vector;
template < class, class > class list;
template < class, class > class forward_list;
template < class, class > class deque;
template < class, class > class queue;
template < class, class, class > class priority_queue;
template < class, class > class stack;
template < class, class, class, class > class map;
template < class, class, class, class > class multimap;
template < class, class, class, class, class > class unordered_map;
template < class, class, class, class, class > class unordered_multimap;
template < class, class, class > class set;
template < class, class, class > class multiset;
template < class, class, class, class > class unordered_set;
template < class, class, class, class > class unordered_multiset;
}

#define DEFINE_HAS_SERIALISATION_FUNCTION( Name, Signature ) \
template < typename T > \
struct HasOn##Name \
{ \
	template < typename U > static long test( std::enable_if_t< std::is_same_v< Signature, FunctionTraits::GetSignature< decltype( &U::On##Name ) > >, int > ); \
	template < typename U > static char test( ... ); \
	static constexpr bool Value = sizeof( test< T >( 0 ) ) == sizeof( long ); \
};

class Serialiser;
class Deserialiser;
class Sizer;

namespace SerialisationTempNameSpaceToStopConflictWithAlreadyExistingISerialisableClassInterfaceThatJesseMade
{
// Implement Serialisation runtime interface.
struct ISerialisable
{
protected:

	friend class Serialisation;

	virtual void OnBeforeSerialise() {}
	virtual void OnAfterSerialise() {}
	virtual void OnSerialise( Serialiser& a_Serialiser ) const = 0;
};

// Implement Deserialisation runtime interface.
struct IDeserialisable
{
protected:

	friend class Serialisation;

	virtual void OnBeforeDeserialise() {}
	virtual void OnAfterDeserialise() {}
	virtual void OnDeserialise( Deserialiser& a_Deserialiser ) = 0;
};

// Implement Sizing runtime interface.
struct ISizeable
{
protected:

	virtual void OnSize( Sizer& a_Sizer ) const = 0;
};
}

class Serialisation
{
public:

	Serialisation() = delete;

private:

	struct Helpers
	{
		template < typename... T >
		struct PriorityQueueHelper : public std::priority_queue< T... >
		{
			using HeapType = decltype( std::priority_queue< T... >::c );

			PriorityQueueHelper( std::priority_queue< T... >&& a_Queue )
				: std::priority_queue< T... >( std::forward< std::priority_queue< T... > >( a_Queue ) )
				, Heap( const_cast< HeapType& >( this->c ) )
			{}

			HeapType& Heap;
		};
	};

	DEFINE_HAS_SERIALISATION_FUNCTION( BeforeSerialise, void() );
	DEFINE_HAS_SERIALISATION_FUNCTION( Serialise, void( Serialiser& ) const );
	DEFINE_HAS_SERIALISATION_FUNCTION( AfterSerialise, void() );
	DEFINE_HAS_SERIALISATION_FUNCTION( Size, void( Sizer& ) const );
	DEFINE_HAS_SERIALISATION_FUNCTION( BeforeDeserialise, void() );
	DEFINE_HAS_SERIALISATION_FUNCTION( Deserialise, void( Deserialiser& ) );
	DEFINE_HAS_SERIALISATION_FUNCTION( AfterDeserialise, void() );

	friend class Serialiser;
	friend class Deserialiser;
	friend class Sizer;

	template < typename T >
	static void Serialise( Serialiser& a_Serialiser, const T& a_Object );

	template < typename T >
	static void Deserialise( Deserialiser& a_Deserialiser, T& a_Object );

	template < typename T >
	static void SizeOf( Sizer& a_Sizer, const T& a_Object );
};

// Given a byte stream that has been allocated beforehand, a Serialiser will automatically serialise any object given to it.
// Order of serialisation is as follows:
// - If a type is an STL container, serialise out the size of the container, and then serialise each element individually.
// - If a type has implemented OnSerialise(Serialiser&) const, then this will be used to serialise the object.
// - If none of the above, the object will be reinterpret casted into a byte stream and written out.
class Serialiser
{
public:
	
	// Default element deserialisation functor for deserialising container types.
	struct DefaultFunctor
	{
		template < typename T >
		void operator()( Serialiser& a_Serialiser, const T& a_Object ) { a_Serialiser << a_Object; }
	};

	Serialiser( byte_t* a_Data )
		: m_Data( a_Data )
		, m_Head( a_Data )
	{}
	
	// Serialise the object as a byte stream.
	Serialiser& SerialiseAsMemory( const void* a_Data, size_t a_Size )
	{
		memcpy( m_Head, a_Data, a_Size );
		m_Head += a_Size;
		return *this;
	}

	// Attempt to serialise the object if it has implemented the OnSerialise interface. Fallback to byte stream serialisation.
	template < typename T >
	Serialiser& SerialiseAsObject( const T& a_Object )
	{
		Serialisation::Serialise( *this, a_Object );
		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename T, typename Functor = DefaultFunctor >
	inline Serialiser& SerialiseAsContainer( const T& a_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::Serialise( *this, a_Container );
		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T >
	Serialiser& SerialiseAsContainer( const std::pair< T... >& a_Container )
	{
		// Write out values.
		SerialiseVariadic( a_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T >
	Serialiser& SerialiseAsContainer( const std::tuple< T... >& a_Container )
	{
		// Write out values.
		SerialiseVariadic( a_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::basic_string< T... >& a_Container )
	{
		auto Size = a_Container.length();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out characters.
		SerialiseAsMemory( a_Container.data(), sizeof( *a_Container.data() ) * a_Container.length() );

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( T( &a_Container )[ N ], Functor&& a_Functor = Functor{} )
	{
		size_t Size = N;

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::array< T, N >& a_Container, Functor&& a_Functor = Functor{} )
	{
		size_t Size = N;

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::vector< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::list< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::forward_list< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = std::distance( a_Container.begin(), a_Container.end() );

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::deque< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Serialiser& SerialiseAsContainer( const std::queue< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		return SerialiseAsContainer( a_Container._Get_container(), std::forward< Functor >( a_Functor ) );
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::priority_queue< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::Helpers::PriorityQueueHelper< T... > Helper( std::move( const_cast< std::priority_queue< T... >& >( a_Container ) ) );
		SerialiseAsContainer( Helper.Heap, std::forward< Functor >( a_Functor ) );
		const_cast< std::priority_queue< T... >& >( a_Container ) = std::move( Helper );
		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Serialiser& SerialiseAsContainer( const std::stack< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		return SerialiseAsContainer( a_Container._Get_container(), std::forward< Functor >( a_Functor ) );
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::map< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::multimap< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::unordered_map< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::unordered_multimap< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::set< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::multiset< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::unordered_set< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to serialise the object as a collection. If not a collection, it will fall back to serialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Serialiser& SerialiseAsContainer( const std::unordered_multiset< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		auto Size = a_Container.size();

		// Write out size.
		SerialiseAsMemory( &Size, sizeof( Size ) );

		// Write out values.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Automatically attempt to detect serialisation strategy. First try to serialise as a container, then as an object, and if those fail, as a byte stream.
	template < typename T >
	inline Serialiser& operator<<( const T& a_ObjectOrContainer )
	{
		return SerialiseAsContainer( a_ObjectOrContainer );
	}

	// Get the data pointer at the begging of the stream.
	inline const byte_t* GetData() const { return m_Data; }

	// Get the data head at the current position of the stream.
	inline const byte_t* GetHead() const { return m_Head; }

	// Get the bytes written out so far.
	inline size_t GetBytesWritten() const { return m_Head - m_Data; }

private:

	template < typename T, size_t... Idx >
	void SerialiseVariadic( const T& a_Object, std::in_place_type_t< std::index_sequence< Idx... > > )
	{
		( ( *this << std::get< Idx >( a_Object ) ), ... );
	}

	byte_t* m_Data;
	byte_t* m_Head;
};

// Given a byte stream full of serialised data, a Deserialiser will automatically deserialise any object given to it.
// Order of deserialisation is as follows:
// - If a type is an STL container, deserialise out the size of the container, resize the container to that size, and then desserialise each element individually.
// - If a type has implemented OnDeserialise(Deserialiser&), then this will be used to deserialise the object.
// - If none of the above, the object will be reinterpret casted into a byte stream and read into.
class Deserialiser
{
public:

	// Default element deserialisation functor for deserialising container types.
	struct DefaultFunctor
	{
		template < typename T >
		void operator()( Deserialiser& a_Deserialiser, T& a_Object ) { a_Deserialiser >> a_Object; }
	};

	Deserialiser( byte_t* a_Data )
		: m_Data( a_Data )
		, m_Head( a_Data )
	{}

	// Deserialise the object as a byte stream.
	Deserialiser& DeserialiseAsMemory( void* o_Data, size_t a_Size )
	{
		memcpy( o_Data, m_Head, a_Size );
		m_Head += a_Size;
		return *this;
	}

	// Attempt to deserialise the object if it has implemented the OnDeserialise interface. Fallback to byte stream deserialisation.
	template < typename T >
	Deserialiser& DeserialiseAsObject( T& o_Object )
	{
		Serialisation::Deserialise( *this, o_Object );
		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( T& o_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::Deserialise( *this, o_Container );
		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T >
	Deserialiser& DeserialiseAsContainer( std::pair< T... >& o_Container )
	{
		// Read in values.
		DeserialiseVariadic( o_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T >
	Deserialiser& DeserialiseAsContainer( std::tuple< T... >& o_Container )
	{
		// Read in values.
		DeserialiseVariadic( o_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::basic_string< T... >& o_Container )
	{
		typename std::basic_string< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Resize the string to that length.
		o_Container.resize( Size );

		// Read in characters.
		DeserialiseAsMemory( o_Container.data(), sizeof( *o_Container.data() ) * Size );

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( T( &o_Container )[ N ], Functor&& a_Functor = Functor{})
	{
		size_t Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		Size = N < Size ? N : Size;

		// Read in each element.
		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, o_Container[ i ] );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::array< T, N >& o_Container, Functor&& a_Functor = Functor{} )
	{
		size_t Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		Size = N < Size ? N : Size;

		// Read in each element.
		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, o_Container[ i ] );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::vector< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::vector< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Reserve the vector.
		o_Container.resize( Size );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, o_Container[ i ] );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::list< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::list< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, o_Container.emplace_back() );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::forward_list< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::forward_list< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		auto Begin = o_Container.before_begin();

		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, *o_Container.emplace_after( Begin ) );
			++Begin;
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::deque< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::deque< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		o_Container.resize( Size );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			a_Functor( *this, o_Container[ i ] );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Deserialiser& DeserialiseAsContainer( std::queue< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		return DeserialiseAsContainer( const_cast< std::remove_const_t< std::remove_reference_t< decltype( o_Container._Get_container() ) > >& >( o_Container._Get_container() ), std::forward< Functor >(a_Functor));
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::priority_queue< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::Helpers::PriorityQueueHelper< T... > Helper( std::move( o_Container ) );
		DeserialiseAsContainer( Helper.Heap, std::forward< Functor >( a_Functor ) );
		o_Container = std::move( Helper );
		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Deserialiser& DeserialiseAsContainer( std::stack< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		return DeserialiseAsContainer( const_cast< std::remove_const_t< std::remove_reference_t< decltype( o_Container._Get_container() ) > >& >( o_Container._Get_container() ), std::forward< Functor >( a_Functor ) );
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::map< T... >& o_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		typename std::map< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::map< T... >::key_type Key;
			typename std::map< T... >::mapped_type Value;
			a_KeyFunctor( *this, Key );
			a_ValueFunctor( *this, Value );
			o_Container.emplace( std::make_pair( std::move( Key ), std::move( Value ) ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::multimap< T... >& o_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		typename std::multimap< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::multimap< T... >::key_type Key;
			typename std::multimap< T... >::mapped_type Value;
			a_KeyFunctor( *this, Key );
			a_ValueFunctor( *this, Value );
			o_Container.emplace( std::make_pair( std::move( Key ), std::move( Value ) ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::unordered_map< T... >& o_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		typename std::unordered_map< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::unordered_map< T... >::key_type Key;
			typename std::unordered_map< T... >::mapped_type Value;
			a_KeyFunctor( *this, Key );
			a_ValueFunctor( *this, Value );
			o_Container.emplace( std::make_pair( std::move( Key ), std::move( Value ) ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::unordered_multimap< T... >& o_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		typename std::unordered_multimap< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::unordered_multimap< T... >::key_type Key;
			typename std::unordered_multimap< T... >::mapped_type Value;
			a_KeyFunctor( *this, Key );
			a_ValueFunctor( *this, Value );
			o_Container.emplace( std::make_pair( std::move( Key ), std::move( Value ) ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::set< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::set< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::set< T... >::value_type Value;
			a_Functor( *this, Value );
			o_Container.emplace( std::move( Value ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::multiset< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::multiset< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::multiset< T... >::value_type Value;
			a_Functor( *this, Value );
			o_Container.emplace( std::move( Value ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::unordered_set< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::unordered_set< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::unordered_set< T... >::value_type Value;
			a_Functor( *this, Value );
			o_Container.emplace( std::move( Value ) );
		}

		return *this;
	}

	// Attempt to deserialise the object as a collection. If not a collection, it will fall back to deserialising it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Deserialiser& DeserialiseAsContainer( std::unordered_multiset< T... >& o_Container, Functor&& a_Functor = Functor{} )
	{
		typename std::unordered_multiset< T... >::size_type Size;

		// Read in size.
		DeserialiseAsMemory( &Size, sizeof( Size ) );

		// Read in values.
		for ( size_t i = 0; i < Size; ++i )
		{
			typename std::unordered_multiset< T... >::value_type Value;
			a_Functor( *this, Value );
			o_Container.emplace( std::move( Value ) );
		}

		return *this;
	}

	// Automatically attempt to detect deserialisation strategy. First try to deserialise as a container, then as an object, and if those fail, as a byte stream.
	template < typename T >
	inline Deserialiser& operator>>( T& o_ObjectOrContainer )
	{
		return DeserialiseAsContainer( o_ObjectOrContainer );
	}

	// Get the data pointer at the begging of the stream.
	inline const byte_t* GetData() const { return m_Data; }

	// Get the data head at the current position of the stream.
	inline const byte_t* GetHead() const { return m_Head; }

	// Get the bytes read so far.
	inline size_t GetBytesRead() const { return m_Head - m_Data; }

private:

	template < typename T, size_t... Idx >
	void DeserialiseVariadic( T& a_Object, std::in_place_type_t< std::index_sequence< Idx... > > )
	{
		( ( *this >> std::get< Idx >( a_Object ) ), ... );
	}

	const byte_t* m_Data;
	const byte_t* m_Head;
};

// Given an object, a Sizer will calculate the serialised size of an object.
// Order of sizing is as follows:
// - If a type is an STL container, size the size type of the container, and then size each element individually.
// - If a type has implemented OnSize(Sizer&) const, then this will be used to size the object.
// - If none of the above, the object will be sized as a byte stream.
class Sizer
{
public:

	// Default element sizer functor for container sizing.
	struct DefaultFunctor
	{
		template < typename T >
		void operator()( Sizer& a_Sizer, const T& a_Object ) { a_Sizer + a_Object; }
	};

	Sizer()
		: m_Size( 0u )
	{}

	// Add explicit size value to the sizer's tally.
	inline Sizer& AddSizeOfMemory( size_t a_Size )
	{
		m_Size += a_Size;
		return *this;
	}

	// Attempt to size the object if it has implemented the OnSize interface. Fallback to byte stream sizing.
	template < typename T >
	Sizer& AddSizeOfObject( const T& a_Object )
	{
		Serialisation::SizeOf( *this, a_Object );
		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const T& a_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::SizeOf( *this, a_Container );
		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T >
	Sizer& AddSizeOfContainer( const std::pair< T... >& a_Container )
	{
		SizeOfVariadic( a_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );
		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T >
	Sizer& AddSizeOfContainer( const std::tuple< T... >& a_Container )
	{
		SizeOfVariadic( a_Container, std::in_place_type< std::make_index_sequence< sizeof...( T ) > > );
		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::basic_string< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.length() ) );

		// Add the size of the entire string.
		AddSizeOfMemory( sizeof( *a_Container.data() ) * a_Container.length() );

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( T( &a_Container )[ N ], Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( N ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename T, size_t N, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::array< T, N >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( N ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::vector< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::list< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::forward_list< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( std::distance( a_Container.begin(), a_Container.end() ) ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::deque< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Sizer& AddSizeOfContainer( const std::queue< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		return AddSizeOfContainer( a_Container._Get_container(), std::forward< Functor >(a_Functor));
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::priority_queue< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		Serialisation::Helpers::PriorityQueueHelper< T... > Helper( std::move( const_cast< std::priority_queue< T... >& >( a_Container ) ) );
		AddSizeOfContainer( Helper.Heap, std::forward< Functor >( a_Functor ) );
		const_cast< std::priority_queue< T... >& >( a_Container ) = std::move( Helper );
		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	inline Sizer& AddSizeOfContainer( const std::stack< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		return AddSizeOfContainer( a_Container._Get_container(), std::forward< Functor >( a_Functor ) );
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::map< T... >& a_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_KeyFunctor( *this, std::get< 0 >( Object ) );
			a_ValueFunctor( *this, std::get< 1 >( Object ) );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::multimap< T... >& a_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_KeyFunctor( *this, std::get< 0 >( Object ) );
			a_ValueFunctor( *this, std::get< 1 >( Object ) );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::unordered_map< T... >& a_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_KeyFunctor( *this, std::get< 0 >( Object ) );
			a_ValueFunctor( *this, std::get< 1 >( Object ) );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename KeyFunctor = DefaultFunctor, typename ValueFunctor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::unordered_multimap< T... >& a_Container, KeyFunctor&& a_KeyFunctor = KeyFunctor{}, ValueFunctor&& a_ValueFunctor = ValueFunctor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_KeyFunctor( *this, std::get< 0 >( Object ) );
			a_ValueFunctor( *this, std::get< 1 >( Object ) );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::set< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::multiset< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::unordered_set< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Attempt to size the object as a collection. If not a collection, it will fall back to sizing it as an object.
	template < typename... T, typename Functor = DefaultFunctor >
	Sizer& AddSizeOfContainer( const std::unordered_multiset< T... >& a_Container, Functor&& a_Functor = Functor{} )
	{
		// Add the size of size.
		AddSizeOfMemory( sizeof( a_Container.size() ) );

		// Add the size of each element.
		for ( const auto& Object : a_Container )
		{
			a_Functor( *this, Object );
		}

		return *this;
	}

	// Automatically attempt to detect sizing strategy. First try to size as a container, then as an object, and if those fail, as a byte stream.
	template < typename T >
	inline Sizer& operator+( const T& a_ObjectOrContainer )
	{
		return AddSizeOfContainer( a_ObjectOrContainer );
	}

	// Implicit cast sizer's tally to a size_t.
	operator size_t () const { return m_Size; }

private:

	template < typename T, size_t... Idx >
	void SizeOfVariadic( T& a_Object, std::in_place_type_t< std::index_sequence< Idx... > > )
	{
		( ( *this + std::get< Idx >( a_Object ) ), ... );
	}

	size_t m_Size;
};

template < typename T >
void Serialisation::Serialise( Serialiser& a_Serialiser, const T& a_Object )
{
	if constexpr ( Serialisation::HasOnBeforeSerialise< T >::Value )
	{
		const_cast< T& >( a_Object ).OnBeforeSerialise();
	}

	if constexpr ( Serialisation::HasOnSerialise< T >::Value )
	{
		a_Object.OnSerialise( a_Serialiser );
	}
	else
	{
		a_Serialiser.SerialiseAsMemory( &a_Object, sizeof( a_Object ) );
	}

	if constexpr ( Serialisation::HasOnAfterSerialise< T >::Value )
	{
		const_cast< T& >( a_Object ).OnAfterSerialise();
	}
}

template < typename T >
void Serialisation::Deserialise( Deserialiser& a_Deserialiser, T& o_Object )
{
	if constexpr ( Serialisation::HasOnBeforeDeserialise< T >::Value )
	{
		const_cast< T& >( o_Object ).OnBeforeDeserialise();
	}

	if constexpr ( Serialisation::HasOnDeserialise< T >::Value )
	{
		o_Object.OnDeserialise( a_Deserialiser );
	}
	else
	{
		a_Deserialiser.DeserialiseAsMemory( &o_Object, sizeof( o_Object ) );
	}

	if constexpr ( Serialisation::HasOnAfterDeserialise< T >::Value )
	{
		const_cast< T& >( o_Object ).OnAfterDeserialise();
	}
}

template < typename T >
void Serialisation::SizeOf( Sizer& a_Sizer, const T& a_Object )
{
	if constexpr ( Serialisation::HasOnSize< T >::Value )
	{
		a_Object.OnSize( a_Sizer );
	}
	else
	{
		a_Sizer.AddSizeOfMemory( sizeof( a_Object ) );
	}
}
