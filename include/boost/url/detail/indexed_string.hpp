//
// Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/CPPAlliance/url
//

#ifndef BOOST_URL_DETAIL_PARTS_STR_HPP_INCLUDED
#define BOOST_URL_DETAIL_PARTS_STR_HPP_INCLUDED

#include <boost/url/detail/storage.hpp>
#include <boost/url/error.hpp>
#include <cstdlib>
#include <cstring>

namespace boost { namespace urls {

namespace detail {

struct empty_base { };

template <int PartCount, class PartsData = empty_base>
class indexed_string;

template <int PartCount, class PartsData = empty_base>
class part_index: public PartsData
{
	static_assert(PartCount > 0, "Invalid PartCount");

	friend class indexed_string<PartCount, PartsData>;

	static constexpr int terminating_zero = PartCount;

	BOOST_URL_NODISCARD
	int
	check_invariants() const noexcept
	{
		for( int i=0; i!=PartCount - 1; ++i )
			if( index_[i] > index_[i+1] )
				return __LINE__;

		return 0;
	}

protected:

    std::size_t index_[PartCount];

	void
	reset() noexcept
	{
		for( int i=0; i!=PartCount; ++i )
			index_[i] = 0;
	}

public:

	part_index()
	{
		reset();

		BOOST_ASSERT( check_invariants() == 0 );
	}

	std::size_t
	offset( int part ) const noexcept
	{
		BOOST_ASSERT( part >= 0 );
		BOOST_ASSERT( part <= PartCount );

		if( part == 0 )
			return 0;

		return index_[part - 1];
	}

	char const*
	ptr( int part, char const* s ) const noexcept
	{
		return s + offset( part );
	}

	char*
	ptr( int part, char* s ) const noexcept
	{
		return s + offset( part );
	}

	string_view
	get( int first, int last, char const* s ) const noexcept
	{
		BOOST_ASSERT( first >= 0 );
		BOOST_ASSERT( first < PartCount );
		BOOST_ASSERT( last >= first );
		BOOST_ASSERT( last <= PartCount );

		return
			{
				ptr( first, s ),
				length( first, last )
			};
	}

	std::size_t
	length_all() const noexcept
	{
		BOOST_ASSERT( check_invariants() == 0 );

		return offset( terminating_zero );
	}

	std::size_t
	length( int first, int last ) const noexcept
	{
		BOOST_ASSERT( first >= 0 );
		BOOST_ASSERT( first <= PartCount );
		BOOST_ASSERT( last >= first );
		BOOST_ASSERT( last <= PartCount );
		BOOST_ASSERT( check_invariants() == 0 );

		return
			offset( last ) -
			offset( first );
	}

	std::size_t
	length( int first ) const noexcept
	{
		return length( first, first + 1 );
	}

	bool
	empty() const noexcept
	{
		return length_all() == 0;
	}

    void
    resize( int part, std::size_t n ) noexcept
    {
		BOOST_ASSERT( part >= 0 );
		BOOST_ASSERT( part < PartCount );

        auto const n0 = length( part );
		for( int i=part; i!=PartCount; ++i )
			index_[i] += // NOTE: Part zero offset not stored in index_ (always 0).
				( n - n0 );
    }

    void
    split( int part, std::size_t n ) noexcept
    {
		BOOST_ASSERT( part >= 0 );
		BOOST_ASSERT( part < PartCount );

        index_[part] = // NOTE: Part zero offset not stored in index_ (always 0).
			offset( part ) + n;
    }
};

template <int PartCount, class PartsData = empty_base>
class indexed_view: public part_index<PartCount, PartsData>
{
	static constexpr int terminating_zero = PartCount;

	using base = part_index<PartCount, PartsData>;

	friend class indexed_string<PartCount, PartsData>;

	BOOST_URL_NODISCARD
	int
	check_invariants() const noexcept
	{
		if( s_ )
		{
			if( this->template offset<terminating_zero>() != std::strlen(s_) )
				return __LINE__;

			for( int i=0; i!=PartCount - 1; ++i )
				if( this->index_[i] > this->index_[i+1] )
					return __LINE__;
		}
		else
		{
			for( int i=0; i!=PartCount; ++i )
				if( this->index_[i] )
					return __LINE__;
		}
		return 0;
	}

protected:

	char const* s_;

public:

	indexed_view() noexcept:
		s_( 0 )
	{
	}

	explicit indexed_view( char const* s ) noexcept:
		s_( s )
	{
	}

	indexed_view( char const* s, base const & pt ) noexcept:
		base( pt ),
		s_( s )
	{
		BOOST_ASSERT( check_invariants() == 0 );
	}

	char const* data() const noexcept
	{
		return s_;
	}

	char const*
	c_str() const noexcept
	{
		if( !s_ )
			return "";

		return s_;
	}

	char const *
	ptr() const noexcept
	{
		return ptr(0);
	}

	char const*
	ptr( int part ) const noexcept
	{
		return base::ptr(
			part,
			s_ );
	}

	string_view
	get( int first, int last ) const noexcept
	{
		return base::get(
			first,
			last,
			s_ );
	}

	string_view
	get( int first ) const noexcept
	{
		return get(
			first,
			first + 1 );
	}

	string_view
	get_all() const noexcept
	{
		return { s_, this->length_all() };
	}

	using base::length_all;
	using base::length;
	using base::empty;
	using base::offset;
};

template <int PartCount, class PartsData>
class indexed_string: public indexed_view<PartCount, PartsData>
{
	indexed_string( indexed_string const& ) = delete;
	indexed_string& operator=( indexed_string const& ) = delete;

	static constexpr int terminating_zero = PartCount;

    storage& a_;
	std::size_t capacity_ = 0;

	BOOST_URL_NODISCARD
	int
	check_invariants() const noexcept
	{
		char const * s = this->s_;
		if( capacity_ )
		{
			if( !s )
				return __LINE__;

			if( this->offset(terminating_zero) != std::strlen(s) )
				return __LINE__;

			for( int i=0; i!=PartCount - 1; ++i )
			{
				if( this->index_[i] > this->index_[i+1] )
					return __LINE__;
				if( this->index_[i] > this->offset(terminating_zero) )
					return __LINE__;
			}
		}
		else
		{
			if( s )
				return __LINE__;

			for( int i=0; i!=PartCount; ++i )
				if( this->index_[i] )
					return __LINE__;
		}
		return 0;
	}

	public:

	explicit indexed_string( storage& a ) noexcept:
		a_(a)
	{
		clear_all();
	}

	~indexed_string() noexcept
	{
		a_.deallocate( ptr(), capacity_ );
	}

	std::size_t
	capacity() const noexcept
	{
		if( capacity_ == 0 )
			return 0;

		BOOST_ASSERT( capacity_ > 1 );
		return capacity_ - 1;
	}

	char*
	reserve_all( std::size_t n )
	{
		BOOST_ASSERT( check_invariants() == 0 );

		if( ++n <= capacity_ ) // One more for the zero terminator.
			return ptr();

		char* s = a_.allocate(n);
		BOOST_ASSERT( s!=0 );

		memcpy( s, this->s_, this->length_all() );
		BOOST_ASSERT( this->offset(terminating_zero) < n );
		s[this->offset(terminating_zero)] = 0;

		a_.deallocate( ptr(), capacity_ );
		this->s_ = s;
		capacity_ = n;

		BOOST_ASSERT( check_invariants() == 0 );

		return s;
	}

	char*
	reserve_more( std::size_t n )
	{
		std::size_t n0 = this->length_all();
		if(n > (
			(std::size_t)-1)- n0)
			too_large::raise();
		return reserve_all( n0 + n );
	}

	void
	reserve( int part, std::size_t n )
	{
		auto const ps = this->length(part);
		if( n > ps )
			reserve_all( this->length_all() + (n - ps) );
	}

	void
	clear_all() noexcept
	{
		this->reset();
		if( char * s = const_cast<char*>(this->s_) )
			s[0] = 0;

		BOOST_ASSERT( check_invariants() == 0 );
	}

	void
	clear( int first, int last ) noexcept
	{
		BOOST_ASSERT( first >= 0 );
		BOOST_ASSERT( first <= PartCount );
		BOOST_ASSERT( last >= first );
		BOOST_ASSERT( last <= PartCount );
		BOOST_ASSERT( check_invariants() == 0 );

		char* const fp = ptr( first );
		char const* const lp = ptr( last );
		if( fp == lp )
			return;

		std::memmove( fp, lp, ptr( terminating_zero ) - lp );

		std::size_t n = fp - this->s_;
		for( int i=first; i!=last; ++i )
			this->index_[i] = n;
		for( int i=last; i!=PartCount; ++i )
			this->index_[i] -= lp - fp;

		*ptr(terminating_zero) = 0;

		BOOST_ASSERT( check_invariants() == 0 );
		BOOST_ASSERT( this->length(first, last) == 0 );
	}

	void
	clear( int part ) noexcept
	{
		clear( part, part+1 );
	}

	char*
	ptr() noexcept
	{
		return ptr(0);
	}

	char*
	ptr( int part ) noexcept
	{
		return part_index<PartCount, PartsData>::ptr(
			part,
			const_cast<char *>(this->s_) );
	}




	char*
	resize(
		int part,
		std::size_t new_size)
	{
		BOOST_ASSERT( part >= 0 );
		BOOST_ASSERT( part <= PartCount );
		char* s = ptr();
		auto const len = this->length(part);
		if(len == new_size)
			return s + this->offset(part);

		// shrink
		if(new_size <= len)
		{
			auto const n = static_cast<
				std::size_t>(len - new_size);
			auto const pos = this->offset(part + 1);
			std::memmove(
				s + pos - n,
				s + pos,
				this->offset(terminating_zero) - pos + 1);
			for(auto i = part;
				i < PartCount; ++i)
				this->index_[i] -= n;

			BOOST_ASSERT( check_invariants() == 0 );
			return s + this->offset(part);
		}

		// grow
		s = reserve_more(
			new_size - len);
		auto const n = static_cast<
			std::size_t>(new_size - len);
		auto const pos =
			this->offset(part + 1);
		std::memmove(
			s + pos + n,
			s + pos,
			this->offset(terminating_zero) -
				pos + 1);
		for(auto i = part;
			i < PartCount; ++i)
			this->index_[i] += n;
		return s + this->offset(part);
	}

	char*
	resize(
		int first,
		int last,
		std::size_t new_size)
	{
		BOOST_ASSERT( first >= 0 );
		BOOST_ASSERT( first <= PartCount );
		BOOST_ASSERT( last > first );
		BOOST_ASSERT( last <= PartCount );

		char* s = ptr();
		auto const len =
			this->length(first, last);
		if(new_size == 0 && len == 0)
			return s + this->offset(first);

		// shrink
		if(new_size <= len)
		{
			auto const n = static_cast<
				std::size_t>(len - new_size);
			auto const pos = this->offset(last);
			std::memmove(
				s + pos - n,
				s + pos,
				this->offset(
					terminating_zero) - pos + 1);
			for(auto i = first;
				i < last-1; ++i)
				this->index_[i] =
					this->index_[last-1] - n;
			for(auto i = last-1;
				i < PartCount; ++i)
				this->index_[i] -= n;
			return s + this->offset(first);
		}

		// grow
		s = reserve_more(
			new_size - len);
		auto const n = static_cast<
			std::size_t>(new_size - len);
		auto const pos =
			this->offset(last);
		std::memmove(
			s + pos + n,
			s + pos,
			this->offset(terminating_zero) -
				pos + 1);
		for(auto i = first;
			i < last-1; ++i)
			this->index_[i] =
				this->index_[last-1] + n;
		for(auto i = last-1;
			i < PartCount; ++i)
			this->index_[i] += n;
		return s + this->offset(first);
	}






	struct change_part_reserved
	{
		int part;
		std::size_t idx;
		std::ptrdiff_t n;
		std::size_t avail;
	};

	change_part_reserved
	reserve_change_part( int part, char const* pos, std::size_t erase_count, std::size_t insert_count )
	{
		BOOST_ASSERT( check_invariants() == 0 );
		std::size_t const i = pos - this->s_;
		BOOST_ASSERT( i >= this->offset(part) );
		BOOST_ASSERT( i <= this->offset(part+1) );

		std::ptrdiff_t const n = insert_count - erase_count;
		if( n > 0 )
			reserve( part, this->length(part) + n );
		return { part, i, n,  insert_count };
	}

	struct target_buffer
	{
		char* p;
		std::size_t avail;

		char* copy( string_view s, std::size_t offset = 0 ) noexcept
		{
			BOOST_ASSERT( offset + s.size() == avail );
			BOOST_ASSERT( std::memchr(s.data(), 0, s.size()) == 0 );
			memcpy( p + offset, s.data(), avail - offset );
			return p;
		}

		template <class Encoder>
		char* encode( Encoder & e, string_view s, std::size_t offset = 0 ) noexcept
		{
			BOOST_ASSERT( offset + e.encoded_size(s) == avail );
			e.encode( p + offset, s );
			BOOST_ASSERT( std::memchr(p + offset, 0, avail - offset) == 0 );
			return p;
		}
	};

	target_buffer
	change_part( change_part_reserved r ) noexcept
	{
		char* const s = ptr();

		char* p = s + r.idx;
		if( r.n == 0 )
			return { p, r.avail };

		if( r.n > 0 )
		{
			BOOST_ASSERT( this->length(r.part) + r.n < capacity_ );
			std::memmove(
				p + r.n,
				p,
				this->length_all() - r.idx );
		}
		else
		{
			BOOST_ASSERT( r.idx - r.n <= this->offset(r.part+1) );
			std::memmove(
				p,
				p - r.n,
				this->length_all() - r.idx + r.n );
		}

		for( int i=r.part; i!=PartCount; ++i )
			this->index_[i] += r.n;

		BOOST_ASSERT( this->offset(terminating_zero) < capacity_ );
		s[this->offset(terminating_zero)] = 0;

		// NOTE: during insert, the invariant that the string does not contain a
		// zero character may break until new data is moved in via buffer::copy.
		return { p, r.avail };
	}

	char*
	change_part( int part, char const* pos, std::size_t erase_count, std::string_view s )
	{
		return change_part(
			reserve_change_part(
				part,
				pos,
				erase_count,
				s.size() ) ).copy( s );
	}

	struct copy_parts_reserved
	{
		indexed_view<PartCount, PartsData> const& src;
		int first;
		int last;
		std::ptrdiff_t n;
	};

	char*
	copy_impl( part_index<PartCount, PartsData> const& pt, string_view s, int first, int last, int buffer_before, int buffer_after )
	{
		BOOST_ASSERT( first >= 0 );
		BOOST_ASSERT( first <= PartCount );
		BOOST_ASSERT( last >= first );
		BOOST_ASSERT( last <= PartCount );
		BOOST_ASSERT( s.data() != this->s_ );
		BOOST_ASSERT( check_invariants() == 0 );

		std::ptrdiff_t const n =
			(buffer_before + buffer_after +
			pt.length(first, last)) -
			this->length(first, last);

		if( n > 0 )
			reserve_all( this->length_all() + n );

		{
			char* p = ptr( last );
			BOOST_ASSERT( p != 0 );
			std::memmove(
				p + n,
				p,
				this->length( last, PartCount ) + 1 );

			std::size_t n1 = this->offset( first );
			std::size_t n2 = pt.offset( first ) - buffer_before;
			for( int i=first; i!=last; ++i )
			{
				std::size_t d = pt.index_[i] - n2;
				n1 += d;
				n2 += d;
				this->index_[i] = n1;
			}

			for( int i=last; i!=PartCount; ++i )
				this->index_[i] += n;
		}

		{
			char* p = ptr( first );
			std::memcpy(
				p + buffer_before,
				s.data() + pt.offset( first ),
				pt.length( first, last ) );
			return p;
		}
	}

	char*
	copy_with_prefix( part_index<PartCount, PartsData> const& pt, string_view s, std::string_view prefix, int first, int last )
	{
		std::size_t const n = prefix.size();
		void* p = std::memcpy(
			copy_impl( pt, s, first, last, n, 0 ),
			prefix.data(),
			n );
		return static_cast<char*>(p);
	}

	char*
	copy_with_prefix( part_index<PartCount, PartsData> const& pt, string_view s, std::string_view prefix, int part )
	{
		return copy_with_prefix( pt, s, prefix, part, part + 1 );
	}

	char*
	copy_with_suffix( part_index<PartCount, PartsData> const& pt, string_view s, std::string_view suffix, int first, int last )
	{
		std::size_t const n = suffix.size();
		void* p = std::memcpy(
			copy_impl( pt, s, first, last, 0, n ) + s.size(),
			suffix.data(),
			n );
		return static_cast<char*>(p);
	}

	char*
	copy_with_suffix( part_index<PartCount, PartsData> const& pt, string_view s, std::string_view suffix, int part )
	{
		return copy_with_suffix( pt, s, suffix, part, part + 1 );
	}

	char*
	copy( part_index<PartCount, PartsData> const& pt, string_view s, int first, int last )
	{
		return copy_impl( pt, s, first, last, 0, 0 );
	}

	char*
	copy( part_index<PartCount, PartsData> const& pt, string_view s, int part )
	{
		return copy( pt, s, part, part + 1 );
	}

	void
	copy( indexed_view<PartCount, PartsData> const& src, int first, int last )
	{
		copy( src, src.get_all(), first, last );
	}

	void
	copy( indexed_view<PartCount, PartsData> const& src, int part )
	{
		copy( src, part, part+1 );
	}

	void
	copy_all( part_index<PartCount, PartsData> const& pt, string_view s )
	{
		BOOST_ASSERT( s.data() != this->s_ );
		BOOST_ASSERT( check_invariants() == 0 );

		if( s.empty() )
		{
			clear_all();
			return;
		}

		std::size_t n = s.size();
		reserve_all( n );

		std::memcpy(
			ptr(),
			s.data(),
			n );

		std::memcpy(
			this->index_,
			pt.index_,
			sizeof(this->index_) );

		*ptr(terminating_zero) = 0;

		BOOST_ASSERT( check_invariants() == 0 );
	}

	void
	copy_all( indexed_view<PartCount> const& src )
	{
		copy_all( src, src.get_all() );
	}
};

}

} }

#endif
