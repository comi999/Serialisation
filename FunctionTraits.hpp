#pragma once
#include <tuple>

//==========================================================================
// Utilities for determining traits of function types.
//==========================================================================
namespace FunctionTraits
{
template < typename T >
struct HasInvocation
{
private:

	template < typename U > static long test( decltype( &U::operator() ) );
	template < typename U > static char test( ... );

public:

	static constexpr bool Value = sizeof( test< T >( 0 ) ) == sizeof( long );
};

template < typename T, bool _HasInvocation = HasInvocation< T >::Value >
struct Info
{
	using Object = T;
	using Return = void;
	using Args = std::tuple<>;
	using Signature = void;
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = false;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename T >
struct Info< T, false >
{
	using Object = T;
	using Return = typename Info< decltype( &T::operator() ) >::Return;
	using Args = typename Info< decltype( &T::operator() ) >::Args;
	using Signature = typename Info< decltype( &T::operator() ) >::Signature;
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = false;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename R, typename... A >
struct Info< R( A... ), false >
{
	using Object = void;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... );
	static constexpr bool IsStatic = true;
	static constexpr bool IsDynamic = false;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename O, typename R, typename... A >
struct Info< R( O::* )( A... ), false >
{
	using Object = O;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... );
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = true;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename O, typename R, typename... A >
struct Info< R( O::* )( A... ) const, false >
{
	using Object = O;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... ) const;
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = true;
	static constexpr bool IsConst = true;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename O, typename R, typename... A >
struct Info< R( O::* )( A... ) volatile, false >
{
	using Object = O;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... ) volatile;
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = true;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = true;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename O, typename R, typename... A >
struct Info< R( O::* )( A... ) const volatile, false >
{
	using Object = O;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... ) const volatile;
	static constexpr bool IsStatic = false;
	static constexpr bool IsDynamic = true;
	static constexpr bool IsConst = true;
	static constexpr bool IsVolatile = true;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename R, typename... A >
struct Info< R( * )( A... ), false >
{
	using Object = void;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... );
	static constexpr bool IsStatic = true;
	static constexpr bool IsDynamic = false;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename R, typename... A >
struct Info< R( & )( A... ), false >
{
	using Object = void;
	using Return = R;
	using Args = std::tuple< A... >;
	using Signature = Return( A... );
	static constexpr bool IsStatic = true;
	static constexpr bool IsDynamic = false;
	static constexpr bool IsConst = false;
	static constexpr bool IsVolatile = false;
	static constexpr bool IsFunction = IsStatic || IsDynamic;
	static constexpr bool IsInvocable = false;
};

template < typename T, size_t _Index >
using GetArgument = std::tuple_element_t< _Index, typename Info< T >::Args >;

template < typename T >
using GetArguments = typename Info< T >::Args;

template < typename T >
using GetReturn = typename Info< T >::Return;

template < typename T >
using GetSignature = typename Info< T >::Signature;

template < typename T >
using GetObject = typename Info< T >::Object;

template < typename T >
static constexpr size_t GetArgumentCount = std::tuple_size_v< typename Info< T >::Args >;

template < typename T >
static constexpr bool IsFunction = Info< T >::IsFunction;

template < typename T >
static constexpr bool IsStatic = Info< T >::IsStatic;

template < typename T >
static constexpr bool IsDynamic = Info< T >::IsDynamic;

template < typename T >
static constexpr bool IsInvocable = Info< T >::IsInvocable;

template < typename T >
static constexpr bool IsConst = Info< T >::IsConst;

template < typename T >
static constexpr bool IsVolatile = Info< T >::IsVolatile;

template < typename T >
static constexpr bool IsConstVolatile = Info< T >::IsConst && Info< T >::IsVolatile;
} // FunctionTraits
