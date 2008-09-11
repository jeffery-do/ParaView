// Included by octree.h

/**\var  template<typename _T, int _d=3, class _A> typedef octree<_T,_d,_A>* octree_node<_T,_d,_A>::octree_pointer
  *\brief Shorthand for a pointer to an octree.
  */

/**\var  template<typename _T, int _d=3, class _A> typedef octree_node<_T,_d,_A>* octree_node<_T,_d,_A>::octree_node_pointer
  *\brief Shorthand for a pointer to an octree node (this class).
  */

/**\var  template<typename _T, int _d=3, class _A> typedef const octree_node<_T,_d,_A>* octree_node<_T,_d,_A>::const_octree_node_pointer
  *\brief Shorthand for a pointer to an immutable octree node (this class).
  */

/**\var  template<typename _T, int _d=3, class _A> typedef octree_node<_T,_d,_A>& octree_node<_T,_d,_A>::octree_node_reference
  *\brief Shorthand for a reference to an octree node (this class).
  */

/**\var  template<typename _T, int _d=3, class _A> typedef const octree_node<_T,_d,_A>& octree_node<_T,_d,_A>::const_octree_node_reference
  *\brief Shorthand for a reference to an immutable octree node (this class).
  */

/**\var  template<typename _T, int _d=3, class _A> typedef _T octree_node<_T,_d,_A>::value_type
  *\brief Shorthand for application data.
  */

/**\var  template<typename _T, int _d=3, class _A> typedef _T* octree_node<_T,_d,_A>::pointer
  *\brief Shorthand for a pointer to application data.
  */

/**\var  template<typename _T, int _d=3, class _A> typedef _T& octree_node<_T,_d,_A>::reference
  *\brief Shorthand for a reference to application data.
  */

/**\var  template<typename _T, int _d=3, class _A> typedef const _T* octree_node<_T,_d,_A>::const_pointer
  *\brief Shorthand for a pointer to immutable application data.
  */

/**\var  template<typename _T, int _d=3, class _A> typedef const _T& octree_node<_T,_d,_A>::const_reference
  *\brief Shorthand for a reference to immutable application data.
  */

/**\var  template<typename _T, int _d=3, class _A> octree_pointer octree_node<_T,_d,_A>::_M_parent
  *\brief The octree that owns this node.
  */

/**\var  template<typename _T, int _d=3, class _A> octree_node_pointer octree_node<_T,_d,_A>::_M_children
  *\brief A pointer to an array of \f$2^{\mathrm{\texttt{\_d}}}\f$ children, or NULL if the node is a leaf node.
  */

/**\var  template<typename _T, int _d=3, class _A> double octree_node<_T,_d,_A>::_M_center[_d]
  *\brief The geometric center point of the octree node.
  */

/**\var  template<typename _T, int _d=3, class _A> double octree_node<_T,_d,_A>::_M_size
  *\brief The length of each side of the hypercube defining the octree node. Also called the size of the node.
  */

/**\var  template<typename _T, int _d=3, class _A> value_type octree_node<_T,_d,_A>::_M_data
  *\brief Application-specific data.
  */

/**\brief Default constructor.
  *
  * The node will be a leaf node (i.e. have no children).
  * The node's application-specific data will not be initialized by this constructor.
  * The node will not have an owning octree -- which means that this constructor should never be used except
  * possibly for serialization.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>::octree_node()
{
  this->_M_parent = 0;
  this->_M_children = 0;
  for ( int i = 0; i < _d; ++i )
    {
    this->_M_center[i] = 0.;
    }
  this->_M_size = 1.;
}

/**\brief Nearly-default constructor.
  *
  * The node will be a leaf node (i.e. have no children).
  * The node's application-specific data will not be initialized by this constructor.
  * This form of the constructor is used by the octree to create the root node of the tree.
  *
  * @param[in] parent The octree owning this node
  * @param[in] nodeCenter A pointer to the center point coordinates of this node.
  * @param[in] length The length of a side of this octree hypercube/voxel/what-have-you.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>::octree_node( octree_pointer parent, const double* nodeCenter, double length )
  : _M_parent( parent )
{
  this->_M_children = 0;
  this->_M_size = length;
  for ( int i = 0; i < _d; ++i )
    {
    this->_M_center[i] = nodeCenter[i];
    }
}

/**\brief Root node constructor.
  *
  * The node will be a leaf node (i.e. have no children).
  * The node's application-specific data will be initialized to the value you pass.
  * This form of the constructor is used by the octree to create the root node of the tree.
  *
  * @param[in] parent The octree owning this node
  * @param[in] nodeCenter A pointer to the center point coordinates of this node.
  * @param[in] length The length of a side of this octree hypercube/voxel/what-have-you.
  * @param[in] data   Application-specific data to copy and store at this node.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>::octree_node( octree_pointer parent, const double* nodeCenter, double length, const value_type& data )
  : _M_parent( parent ), _M_data( data )
{
  this->_M_children = 0;
  this->_M_size = length;
  for ( int i = 0; i < _d; ++i )
    {
    this->_M_center[i] = nodeCenter[i];
    }
}

/**\brief Finally, a useful constructor (you may specify application data to be stored with the node).
  *
  * The node will be a leaf node (i.e. have no children).
  * @param[in] parent A reference to the octree node owning this node
  * @param[in] which An integer specifying which child of the \a parent this node is.
  * @param[in] data Application-specific information to be stored at this node.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>::octree_node( octree_node_pointer parent, int which, const value_type& data )
  : _M_parent( parent->_M_parent ), _M_data( data )
{
  this->_M_children = 0;
  this->_M_size = parent->_M_size * 0.5;
  for ( int i = 0; i < _d; ++i )
    {
    this->_M_center[i] = parent->_M_center[i] + ( ( which & (1<<i) ) ? 0.5 : -0.5 ) * this->_M_size;
    }
}

/**\brief Destructor.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>::~octree_node()
{
  if ( this->_M_children )
    delete [] this->_M_children;
}

/**\fn template< typename _T, int _d, typename _A > bool octree_node<_T,_d,_A>::is_leaf_node()
  *\brief Test whether the node has any children.
  *
  * @retval True if the node is a leaf node (no children), false if the node has children.
  */

/**\fn template< typename _T, int _d, typename _A > int octree_node<_T,_d,_A>::num_children()
  *\brief Return the number of direct children of this node.
  *
  * For leaf nodes, this returns 0. Otherwise it will return \f$2^{\mathrm{\texttt{\_d}}}\f$.
  *
  * @retval The number of children.
  */

/**\brief Create children of this node if they did not already exist.
  *
  * Note that the application-specific data is not initialized by this call; you are responsible for any initialization.
  * Use this variant when the default constructor of your application-specific data does some meaningful initialization.
  * @retval     True if children were created, false if they already existed.
  */
template< typename _T, int _d, typename _A >
bool octree_node<_T,_d,_A>::add_children()
{
  if ( this->_M_children )
    {
    return false;
    }
  this->_M_children = new octree_node<_T,_d,_A>[1<<_d];
  for ( int i = 0; i < (1<<_d); ++i )
    {
    octree_node_pointer child = this->_M_children + i;
    child->_M_parent = this->_M_parent;
    child->_M_size = this->_M_size * 0.5;
    for ( int j = 0; j < _d; ++j )
      child->_M_center[j] = this->_M_center[j] + ( ( i & (1<<j) ) ? 0.5 : -0.5 ) * child->_M_size;
    }
  return true;
}

/**\brief Create children of this node if they did not already exist.
  *
  * Note that the application-specific data is not initialized by this call; you are responsible for any initialization.
  * Use this variant when you need to provide a default value for the children's application-specific data.
  * This is useful when the application-specific data is a pointer or some other basic type that does not
  * normally get initialized.
  * @retval     True if children were created, false if they already existed.
  */
template< typename _T, int _d, typename _A >
bool octree_node<_T,_d,_A>::add_children( const _T& child_initializer )
{
  if ( this->_M_children )
    {
    return false;
    }
  this->_M_children = new octree_node<_T,_d,_A>[1<<_d];
  for ( int i = 0; i < (1<<_d); ++i )
    {
    octree_node_pointer child = this->_M_children + i;
    child->_M_parent = this->_M_parent;
    child->_M_data = child_initializer;
    child->_M_size = this->_M_size * 0.5;
    for ( int j = 0; j < _d; ++j )
      child->_M_center[j] = this->_M_center[j] + ( ( i & (1<<j) ) ? 0.5 : -0.5 ) * child->_M_size;
    }
  return true;
}

/**\brief Remove any child nodes from this node (if any existed to begin with).
  *
  * @retval     True if children were removed, false if the node had none to begin with.
  */
template< typename _T, int _d, typename _A >
bool octree_node<_T,_d,_A>::remove_children()
{
  if ( this->_M_children )
    {
    delete [] this->_M_children;
    this->_M_children = 0;
    return true;
    }
  return false;
}

/**\fn template< typename _T, int _d, typename _A > const double* octree_node<_T,_d,_A>::center() const
  *\brief Retrieve the geometric center of a node.
  *
  * Note that this, along with size() provide a way to compute the bounds of the node.
  * @retval A pointer to the node center coordinate array.
  */

/**\fn template< typename _T, int _d, typename _A > reference octree_node<_T,_d,_A>::size() const
  *\brief Retrieve the size (i.e., the length of any side) of a node.
  *
  * Note that this, along with center() provide a way to compute the bounds of the node.
  *
  * \warning Some people refer to the diagonal length as the octree node size; this is <b>not</b> how we use size.
  *          If you would like the diagonal length, multiply size() by \f$\sqrt{3}\f$.
  *
  * @retval The length of a side of the node.
  */

/**\fn template< typename _T, int _d, typename _A > reference octree_node<_T,_d,_A>::value()
  *\brief Retrieve the application-specific data stored at a node.
  *
  * @retval A reference to the application-specific data.
  */

/**\fn template< typename _T, int _d, typename _A > const reference octree_node<_T,_d,_A>::value() const
  *\brief Retrieve a reference to immutable application-specific data stored at a node.
  *
  * @retval A reference to immutable application-specific data.
  */

/**\brief Return a (const) reference to a child node.
  *
  * @param child An integer in \f$[0,2^{\mathrm{\texttt{\_d}}}]\f$ indicating which child node to return.
  */
template< typename _T, int _d, typename _A >
const octree_node<_T,_d,_A>& octree_node<_T,_d,_A>::operator [] ( int child ) const
{
  if ( ! this->_M_children )
    {
    throw vtkstd::domain_error( "Attempt to access children of an octree leaf node." );
    }
  return this->_M_chilren[child];
}

/**\brief Return a reference to a child node.
  *
  * @param child An integer in \f$[0,2^{\mathrm{\texttt{\_d}}}]\f$ indicating which child node to return.
  */
template< typename _T, int _d, typename _A >
octree_node<_T,_d,_A>& octree_node<_T,_d,_A>::operator [] ( int child )
{
  if ( ! this->_M_children )
    {
    throw vtkstd::domain_error( "Attempt to access children of an octree leaf node." );
    }
  return this->_M_children[child];
}

/**\fn  template<typename _T, int _d=3, class _A> reference octree_node<_T,_d,_A>::operator * ()
  *\brief Provide access to the application-specific data.
  */

/**\fn  template<typename _T, int _d=3, class _A> const_reference octree_node<_T,_d,_A>::operator * () const
  *\brief Provide read-only access to the application-specific data.
  */

