<?php

/**
 * Defines a default cache implementation.
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
   * Constructs a DrupalDatabaseCache object.
   *
   * @param $bin
   *   The cache bin for which the object is created.
   */
  public function __construct($bin) {
    parent::__construct($bin);
  }

  /**
   * Implements DrupalCacheInterface::get().
   * @todo
   *   - set the cache locally if it exists in the db.
   */
  public function get($cid) {
    if (empty($cid)) return;

    $cache = apcu_fetch($cid);

    // If the local storage is empty, check the DB.
    if (empty($cache)) {
      $cids = array($cid);
      $cache = parent::getMultiple($cids);
      $cache = !empty($cache[$cid]) ? $cache[$cid] : FALSE;
      // If the db has a copy, store it locally as well.
      //$this->setLocal($cid, $cache, $cache->expire);
      return $cache;
    }

    // If the DB is empty return nothing.
    if (empty($cache)) {
      return FALSE;
    }

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
    //print "setting 1: " . $cid . "\n";
    // APCu is a key-value store - always serialize the data.
    $cache_data = array(
      'data' => serialize($data),
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
  public function setLocal($cid, $data, $expire = CACHE_PERMANENT) {
    // APCu is a key-value store - always serialize the data.
    $cache_data = array(
      'data' => serialize($data),
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
    if (!empty($cid)) {
      apcu_delete($cid);
    }

    if ($wildcard) {
      apcu_clear_cache();
    }

    // Clear the DB cache.
    parent::clear($cid, $wildcard);
  }

  /**
   * Implements DrupalCacheInterface::isEmpty().
   */
  public function isEmpty() {
    //$cache = apcu_cache_info();
  }
}