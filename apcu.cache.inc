<?php

/**
 * Defines an APCU based cache implementation.
 *
 * This cache is based on local APCu data stores and the database. 
 * Every time a cache item gets updated, we write it to the local cache and the DB.
 */
class ApcuCache extends DrupalDatabaseCache implements DrupalCacheInterface {

  /**
   * The cache bin.
   */
  protected $bin;

  /**
   * Constructs an ApcuCache object.
   *
   * @param $bin
   *   The cache bin for which the object is created. This will be mostly for the DB cache.
   */
  public function __construct($bin) {
    parent::__construct($bin);
  }

  /**
   * Implements DrupalCacheInterface::get().
   * @todo: take into account individual cache expiration period.
   */
  public function get($cid) {
    if (empty($cid)) return;

    $cache = apcu_fetch($cid);
    //print "\n\nFrom APCu:\n";
    //print_r($cache);

    // If the local storage is empty, check the DB.
    if (empty($cache)) {
      $cids = array($cid);
      $cache = parent::getMultiple($cids);
      $cache = !empty($cache[$cid]) ? $cache[$cid] : FALSE;
      
      if (empty($cache)) {
        return FALSE;
      }

      //print "\n\nFrom Database:\n";
      //print_r($cache);

      // If the DB has a copy, store it locally as well. 
      // Future calls don't need to go to the DB.
      $this->saveLocally($cid, $cache->data, $cache->expire);
      return $cache;
    }

    // APCu returns arrays, not objects. Drupal needs objects.
    if (!is_object($cache)) {
      $cache = (object) $cache;  
    }

    if ($cache->serialized) {
      $cache->data = unserialize($cache->data);  
    }

    return $cache;
  }

  /**
   * Implements DrupalCacheInterface::getMultiple().
   */
  public function getMultiple(&$cids) {
    if (empty($cids)) return;

    $cache = array();
    foreach ($cids as $cid) {
      $c = $this->get($cid);
      if (!empty($c)) {
        $cache[$cid] = $c;
      }
    }

    return $cache;
  }

  /**
   * Implements DrupalCacheInterface::set().
   */
  public function set($cid, $data, $expire = CACHE_PERMANENT) {
    $cache_data = array(
      'serialized' => 0,
      'created' => REQUEST_TIME,
      'expire' => $expire,
    );

    if (!is_string($data)) {
      $cache_data['data'] = serialize($data);
      $cache_data['serialized'] = 1;
    }
    else {
      $cache_data['data'] = $data;
      $cache_data['serialized'] = 0;
    }

    // Store the data to the local cache;
    apcu_store($cid, $cache_data, $expire);

    // Store the data in the DB as well.
    parent::set($cid, $data, $expire);
  }

  /**
   * Sets the data only in the local store.
   */
  public function saveLocally($cid, $data, $expire = CACHE_PERMANENT) {
    // APCu is a key-value store - always serialize the data.
    $cache_data = array(
      'created' => REQUEST_TIME,
      'expire' => $expire,
    );

    if (!is_string($data)) {
      $cache_data['data'] = serialize($data);
      $cache_data['serialized'] = 1;
    }
    else {
      $cache_data['data'] = $data;
      $cache_data['serialized'] = 0;
    }

    // Store the data to the local cache;
    apcu_store($cid, $cache_data, $expire);
  }

  /**
   * Implements DrupalCacheInterface::clear().
   * @todo
   *   - take into account CACHE_PERMANENT/CACHE_TEMPORARY
   *   - take into account the minimum cache lifetime
   */
  public function clear($cid = NULL, $wildcard = FALSE) {
    // Delete from local storage.
    if ($wildcard || $cid == '*') {
      //print "\n Clearing cache wilcard: $cid \n";
      apcu_clear_cache();
    }

    if (!empty($cid) && $cid != '*') {
      //print "\n Clearing cache $cid \n";
      apcu_delete($cid);
    }

    // Clear the DB cache.
    parent::clear($cid, $wildcard);
  }

  /**
   * Implements DrupalCacheInterface::isEmpty().
   */
  public function isEmpty() {
    $cache = apcu_cache_info();
    if (empty($cache['cache_list'])) {
      return TRUE;
    }
    return FALSE;
  }
}
