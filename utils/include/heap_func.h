/** 
 *  ==============================================================================
 * 
 *          \file   heap_func.h
 *
 *        \author   chenghuige@gmail.com
 *          
 *          \date   2009-11-28 21:15:48.197552
 *  
 *   Description:   add shift down and modify some
 *                  make heap and delete elem functions
 *  ==============================================================================
 */

#ifndef HEAP_FUNC_H_
#define HEAP_FUNC_H_

namespace glzip {

template<typename _RandomAccessIterator, typename _Tp>
    inline void
    heap_shift_down(_RandomAccessIterator __first, _RandomAccessIterator __last, _Tp __value)
    {
      typedef typename iterator_traits<_RandomAccessIterator>::difference_type
	_Distance;
      
      shift_down(__first, _Distance(0), _Distance(__last - __first),
			 __value);
    }

 template<typename _RandomAccessIterator, typename _Tp, typename _Compare>
    inline void
    heap_shift_down(_RandomAccessIterator __first, _RandomAccessIterator __last,
	        _Tp __value, _Compare __comp)
    {
      typedef typename iterator_traits<_RandomAccessIterator>::difference_type
	_Distance;
      shift_down(__first, _Distance(0), _Distance(__last - __first),
			 __value, __comp);
    }

//shift down 操作，将__first + __holeIndex位置的元素
//(其值为__value如果原来不是就相当于先赋值为__value)
//向下shift down,最远到达__first + len - 1位置
template<typename _RandomAccessIterator, typename _Distance, typename _Tp>
void  shift_down(_RandomAccessIterator __first, _Distance __holeIndex,
  	                _Distance __len, _Tp __value)
{
  _Distance __secondChild = 2 * __holeIndex + 2; //右子节点index
  
  while (__secondChild < __len) {
    //执行后secondChild代表两个子节点中较大的节点
    if ( *(__first + __secondChild) < *(__first + (__secondChild - 1)) )
      __secondChild--;
    
    //如果比子节点小
    if ( __value < *(__first + __secondChild)) {
      *(__first + __holeIndex) =  *(__first + __secondChild);
      __holeIndex = __secondChild;         //继续shift down
      __secondChild = 2 * __holeIndex+ 2;
    }
    else 
      break;  
  }
  
  //最后一层可能存在只有左子节点情况
  if (__secondChild == __len) {
    __secondChild--;
   if ( __value < *(__first + __secondChild)) {
      *(__first + __holeIndex) =  *(__first + __secondChild);
      __holeIndex = __secondChild;
   }
  }
  
  //将__value赋值到最后确定的位置
  *(__first + __holeIndex) = __value;   
}
//向下shift down,最远到达__first + len - 1位置
template<typename _RandomAccessIterator, typename _Distance, typename _Tp, typename _Compare>
void  shift_down(_RandomAccessIterator __first, _Distance __holeIndex,
  	                _Distance __len, _Tp __value, _Compare __comp )
{
  _Distance __secondChild = 2 * __holeIndex + 2; //右子节点index
  
  while (__secondChild < __len) {
    //执行后secondChild代表两个子节点中较大的节点
    if ( __comp(*(__first + __secondChild) , *(__first + (__secondChild - 1))) )
      __secondChild--;
    
    //如果比子节点小
    if (__comp( __value , *(__first + __secondChild))) {
      *(__first + __holeIndex) =  *(__first + __secondChild);
      __holeIndex = __secondChild;         //继续shift down
      __secondChild = 2 * __holeIndex+ 2;
    }
    else 
      break;  
  }
  
  //最后一层可能存在只有左子节点情况
  if (__secondChild == __len) {
    __secondChild--;
   if ( __comp(__value , *(__first + __secondChild))) {
      *(__first + __holeIndex) =  *(__first + __secondChild);
      __holeIndex = __secondChild;
   }
  }
  
  //将__value赋值到最后确定的位置
  *(__first + __holeIndex) = __value;   
}
  
  template<typename _RandomAccessIterator, typename _Tp>
    inline void
    __pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
	       _RandomAccessIterator __result, _Tp __value)
    {
      typedef typename iterator_traits<_RandomAccessIterator>::difference_type
	_Distance;
      *__result = *__first;
      shift_down(__first, _Distance(0), _Distance(__last - __first),
			 __value);
    }

 template<typename _RandomAccessIterator>
    inline void
    pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
    {
      typedef typename iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      
      __pop_heap(__first, __last - 1, __last - 1,
		      _ValueType(*(__last - 1)));
    }
  
template<typename _RandomAccessIterator, typename _Tp, typename _Compare>
    inline void
    __pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
	       _RandomAccessIterator __result, _Tp __value, _Compare __comp)
    {
      typedef typename iterator_traits<_RandomAccessIterator>::difference_type
	_Distance;
      *__result = *__first;
      shift_down(__first, _Distance(0), _Distance(__last - __first),
			 __value, __comp);
    }

  template<typename _RandomAccessIterator, typename _Compare>
    inline void
    pop_heap(_RandomAccessIterator __first,
	     _RandomAccessIterator __last, _Compare __comp)
    {
      // concept requirements
      __glibcxx_function_requires(_Mutable_RandomAccessIteratorConcept<
	    _RandomAccessIterator>)
      __glibcxx_requires_valid_range(__first, __last);
      __glibcxx_requires_heap_pred(__first, __last, __comp);

      typedef typename iterator_traits<_RandomAccessIterator>::value_type
	_ValueType;
      __pop_heap(__first, __last - 1, __last - 1,
		      _ValueType(*(__last - 1)), __comp);
    }

template<typename _RandomAccessIterator>
void make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
{
  typedef typename iterator_traits<_RandomAccessIterator>::value_type
  _ValueType;
  typedef typename iterator_traits<_RandomAccessIterator>::difference_type
  _DistanceType;

  if (__last - __first < 2)
    return;

  const _DistanceType __len = __last - __first;
  _DistanceType __parent = (__len - 2) / 2;
  while (true)
  {
    shift_down(__first, __parent, __len, 
                       _ValueType(*(__first + __parent)));
    if (__parent == 0)
      return;
    __parent--;
  }
}

}  //----end of namespace glzip

#endif  //----end of HEAP_FUNC_H_
