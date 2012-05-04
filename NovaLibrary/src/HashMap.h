#ifndef NOVAHASH_H_
#define NOVAHASH_H_

#include <google/dense_hash_map>
#include <exception>

namespace Nova
{

// All hash map exceptions can be caught with this
class hashMapException : public std::exception {};

// Invalid key access exceptions cat be caught with this
class emptyKeyException: public hashMapException
{
  virtual const char* what() const throw()
  {
    return "Unable to access empty key of google hash map";
  }
};

class deleteKeyException: public hashMapException
{
  virtual const char* what() const throw()
  {
    return "Unable to access delete key of google hash map";
  }
};

class emptyKeyNotSetException: public hashMapException
{
	virtual const char* what() const throw()
	{
		return "An empty key was not set on this table";
	}
};


template <class KeyType, class ValueType, class HashFcn, class EqualKey>
class HashMap
{
public:
	HashMap();
	~HashMap();

	// Exception enabled access method
	ValueType& operator[](KeyType key);

	void set_deleted_key(KeyType key);
	void set_empty_key(KeyType key);

	bool keyExists(KeyType key);
	void erase(KeyType key);

	// Expose generic methods we use
	void clear();
	void clear_no_resize();
	void resize(size_t newSize);
	uint size();
	bool empty();

	// Expose the iterators
	typedef typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator iterator;
	typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator begin();
	typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator end();
	typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator find(KeyType key);

private:
	google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey> m_map;

	KeyType m_emptyKey;
	KeyType m_deletedKey;


	EqualKey m_equalityChecker;

	bool m_isDeletedKeyUsed;
	bool m_isEmptyKeyUsed;
};

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
HashMap<KeyType,ValueType,HashFcn,EqualKey>::HashMap()
{
	m_isDeletedKeyUsed = false;
	m_isEmptyKeyUsed = false;
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
HashMap<KeyType,ValueType,HashFcn,EqualKey>::~HashMap()
{
	// Anything we need to do on destruction goes here
}




template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::set_deleted_key(KeyType key)
{
	m_isDeletedKeyUsed = true;

	m_deletedKey = key;
	m_map.set_deleted_key(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::set_empty_key(KeyType key)
{
	m_isEmptyKeyUsed = true;

	m_emptyKey = key;
	m_map.set_empty_key(key);
}


template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::begin()
{
	return m_map.begin();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::end()
{
	return m_map.end();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
typename google::dense_hash_map<KeyType, ValueType, HashFcn, EqualKey>::iterator HashMap<KeyType,ValueType,HashFcn,EqualKey>::find(KeyType key)
{
	return m_map.find(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::clear()
{
	m_map.clear();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::resize(size_t size)
{
	m_map.resize(size);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::erase(KeyType key)
{
	m_map.erase(key);
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
uint HashMap<KeyType,ValueType,HashFcn,EqualKey>::size()
{
	return m_map.size();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
bool HashMap<KeyType,ValueType,HashFcn,EqualKey>::empty()
{
	return m_map.empty();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
void HashMap<KeyType,ValueType,HashFcn,EqualKey>::clear_no_resize()
{
	m_map.clear_no_resize();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
bool HashMap<KeyType,ValueType,HashFcn,EqualKey>::keyExists(KeyType key)
{
	return m_map.find(key) != m_map.end();
}

template<class KeyType, class ValueType, class HashFcn, class EqualKey>
ValueType& HashMap<KeyType,ValueType,HashFcn,EqualKey>::operator[](KeyType key)
{
	if (!m_isEmptyKeyUsed)
	{
		throw emptyKeyNotSetException();
	}
	else if (m_isEmptyKeyUsed && m_equalityChecker.operator()(key, m_emptyKey))
	{
		throw emptyKeyException();
	}
	else if (m_isDeletedKeyUsed && m_equalityChecker.operator()(key, m_deletedKey))
	{
		throw deleteKeyException();
	}
	else
	{
		return m_map[key];
	}
}

}

#endif /* NOVAHASH_H_ */
