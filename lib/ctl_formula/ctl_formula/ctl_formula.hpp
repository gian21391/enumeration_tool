/* MIT License
 *
 * Copyright (c) 2019 Gianluca Martino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <vector>
#include <utility>
#include <iostream>
#include <queue>
#include <memory>

namespace enumeration
{

enum ctl_operator
{
  Constant = 0,
  Var,
  And,
  EX,
  EG,
  EU
};

class ctl_node_pointer
{
public:
  using internal_type = uint32_t;

  ctl_node_pointer() = default;

  ctl_node_pointer( uint32_t index, bool weight )
    : weight( weight )
    , index( index )
  {
  }

  union
  {
    struct
    {
      uint32_t weight : 1;
      uint32_t index  : 31;
    };
    uint32_t data;
  };

  bool operator==( ctl_node_pointer const& other ) const
  {
    return data == other.data;
  }
}; /* ctl_node_pointer */

class ctl_node
{
public:
  using pointer_type = ctl_node_pointer;

  std::array<pointer_type, 2u> children;
  std::array<uint32_t, 2u> data;

  bool operator==( ctl_node const &other ) const
  {
    return children == other.children;
  }
};

}

namespace std
{

template<>
struct hash<enumeration::ctl_node>
{
  uint64_t operator()( enumeration::ctl_node const& node ) const noexcept
  {
    uint64_t seed = -2011;
    seed += node.children[0u].index * 7937;
    seed += node.children[1u].index * 2971;
    seed += node.children[0u].weight * 911;
    seed += node.children[1u].weight * 353;
    seed += node.data[0u] * 911;
    seed += node.data[1u] * 353;
    return seed;
  }
};

} /* namespace std */


namespace enumeration
{

class ctl_storage
{
public:
  ctl_storage()
  {
    nodes.reserve( 10000u );
    hash.reserve( 10000u );

    /* first node reserved for a constant */
    nodes.emplace_back();
    nodes[0].children[0].data = ctl_operator::Constant;
    nodes[0].children[1].data = 0;
  }

  using node_type = ctl_node;

  std::vector<node_type> nodes;
  std::vector<uint32_t> inputs;
  std::vector<typename node_type::pointer_type> outputs;

  std::unordered_map<node_type, uint32_t> hash;

  uint32_t num_pis{0};
}; /* ltl_storage */

} /* namespace enumeration */

namespace enumeration
{

class ctl_formula_store
{
public:
  using node = uint32_t;

  struct ctl_formula
  {
    ctl_formula() = default;

    ctl_formula( uint32_t index, uint32_t complement )
      : complement( complement ), index( index )
    {
    }

    explicit ctl_formula( uint32_t data )
      : data( data )
    {
    }

    ctl_formula( ctl_storage::node_type::pointer_type const& p )
      : complement( p.weight )
      , index( p.index )
    {
    }

    ctl_formula operator!() const
    {
      return ctl_formula{data ^ 1};
    }

    bool operator==( ctl_formula const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( ctl_formula const& other ) const
    {
      return data != other.data;
    }

    union
    {
      struct
      {
        uint32_t complement :  1;
        uint32_t index      : 31;
      };
      uint32_t data;
    };

    operator ctl_storage::node_type::pointer_type() const
    {
      return { index, (bool)complement };
    }
  };

public:

  ctl_formula_store()
          : storage( std::make_shared<ctl_storage>() )
  {
  }

  ctl_formula_store( std::shared_ptr<ctl_storage> const& storage )
          : storage( storage )
  {
  }

  //region formula creation

  ctl_formula get_constant( bool value )
  {
    return {0, value};
  }

  void create_formula( ctl_formula const& a )
  {
    storage->outputs.emplace_back( a );
  }

  bool formula_exists( const ctl_formula& a)
  {
    for (const auto& output : storage->outputs) {
      if (output == a) return true;
    }
    return false;
  }

  ctl_formula create_variable()
  {
    const auto index = node( storage->nodes.size() );
    auto& node = storage->nodes.emplace_back();
    node.data[0] = ctl_operator::Var;
    node.data[1] = storage->inputs.size();

    storage->inputs.emplace_back( index );
    ++storage->num_pis;
    return {index, false};
  }

  ctl_formula create_and( ctl_formula a, ctl_formula b )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : get_constant( false );
    }
    else if ( a.index == 0 )
    {
      return a.complement ? b : get_constant( false );
    }

    ctl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = b;
    n.data[0] = ctl_operator::And;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,false};
  }

  ctl_formula create_or( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto formula = create_and( !f0, !f1 );
    return !formula;
  }

  ctl_formula create_implies( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto formula = create_or( !f0, f1 );
    return formula;
  }

  ctl_formula create_exclusive_or( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto or1 = create_or( f0, f1 );
    auto and1 = create_and( f0, f1 );
    auto and2 = create_and( or1, !and1 );
    return and2;
  }

  ctl_formula create_equals( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto xor1 = create_exclusive_or( f0, f1 );
    return !xor1;
  }

  ctl_formula create_EX( const ctl_formula& a )
  {
    /* trivial cases */
    if ( a.index == 0 )
    {
      return a.complement ? get_constant( true ) : get_constant( false );
    }

    ctl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = {0, 0};
    n.data[0] = ctl_operator::EX;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index,0};
  }

  ctl_formula create_EF( const ctl_formula& f )
  {
    auto eu = create_EU( get_constant( true ), f );
    return eu;
  }

  ctl_formula create_EG( const ctl_formula& a )
  {
    /* trivial cases */
    if ( a.index == 0 )
    {
      return a.complement ? get_constant( true ) : get_constant( false );
    }

    ctl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = {0, 0};
    n.data[0] = ctl_operator::EG;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index, 0};
  }

  ctl_formula create_EU( const ctl_formula& a, const ctl_formula& b )
  {
    // TODO: verify this
    // if "a" is false the whole formula is false
    if ( a == get_constant(false) )
    {
      return get_constant( false );
    }

    ctl_storage::node_type n;
    n.children[0] = a;
    n.children[1] = b;
    n.data[0] = ctl_operator::EU;
    n.data[1] = 0;

    const auto it = storage->hash.find( n );
    if ( it != std::end( storage->hash ) )
    {
      return {it->second, 0};
    }

    const auto index = node( storage->nodes.size() );
    if ( index >= .9 * storage->nodes.capacity() )
    {
      storage->nodes.reserve( uint32_t( 3.1415f * index ) );
      storage->hash.reserve( uint32_t( 3.1415f * index ) );
    }

    storage->nodes.push_back( n );
    storage->hash[n] = index;

    return {index, 0};
  }

  ctl_formula create_ER( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto until = create_AU( !f0, !f1 );
    return !until;
  }

  ctl_formula create_EW( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto eg = create_EG( f0 );
    auto eu = create_EU( f0, f1 );
    return create_or( eg, eu );
  }

  ctl_formula create_AX( const ctl_formula& f )
  {
    auto formula = create_EX( !f );
    return !formula;
  }

  ctl_formula create_AF( const ctl_formula& f )
  {
    auto formula = create_EG( !f );
    return !formula;
  }

  ctl_formula create_AG( const ctl_formula& f )
  {
    auto formula = create_EF( !f );
    return !formula;
  }

  ctl_formula create_AU( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto conj = create_and( !f0, !f1 );
    auto until = create_EU( !f1, conj );
    auto eg = create_EG( !f1 );
    auto and_node = create_and( !until, !eg );
    return and_node;
  }

  ctl_formula create_AR( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto eu = create_EU( !f0, !f1 );
    return !eu;
  }

  ctl_formula create_AW( const ctl_formula& f0, const ctl_formula& f1 )
  {
    auto and1 = create_and( f0, f1 );
    auto eu = create_EU( !f1, !and1 );
    return !eu;
  }
  //endregion

  bool is_constant( node const& n ) const
  {
    assert( storage->nodes[0].data[0] == ctl_operator::Constant );
    return n == 0;
  }

  bool is_variable( node const& n ) const
  {
    return storage->nodes[n].data[0] == ctl_operator::Var && storage->nodes[n].data[1] < static_cast<uint32_t>(storage->num_pis);
  }

  bool is_and( node const& n ) const
  {
    return storage->nodes[n].data[0] == ctl_operator::And;
  }

  bool is_EX( node const& n ) const
  {
    return storage->nodes[n].data[0] == ctl_operator::EX;
  }

  bool is_EG( node const& n ) const
  {
    return storage->nodes[n].data[0] == ctl_operator::EG;
  }

  bool is_EU( node const& n ) const
  {
    return storage->nodes[n].data[0] == ctl_operator::EU;
  }

  node get_node( ctl_formula const& f ) const
  {
    return f.index;
  }

  bool is_complemented( ctl_formula const& f ) const
  {
    return f.complement;
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    for ( auto i = 0u; i < 2u; ++i )
    {
      fn( ctl_formula{storage->nodes[n].children[i]}, i );
    }
  }

  template<typename Fn>
  void foreach_formula( Fn&& fn ) const
  {
    auto begin = storage->outputs.begin(), end = storage->outputs.end();
    while ( begin != end )
    {
      if ( !fn( *begin++ ) )
      {
        return;
      }
    }
  }

protected:
  std::shared_ptr<ctl_storage> storage;
};

}

namespace std
{

template<>
struct hash<enumeration::ctl_formula_store::ctl_formula>
{
  uint64_t operator()( enumeration::ctl_formula_store::ctl_formula const &f ) const noexcept
  {
    uint64_t k = f.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

} /* namespace std */
