<?php

namespace Drupal\acquia_lift\Queue;

use Drupal\Core\Queue\DatabaseQueue;

/**
 * Class AcquiaLiftQueue
 *
 * This class is used for queueing http requests to Acquia Lift. It
 * prevents duplicate items from being added during a request, which
 * can happen when there are multiple calls to the agent save or option
 * set save or delete functions.
 */
class AcquiaLiftQueue extends DatabaseQueue {

  public static $items = array();

  /**
   * Overrides SystemQueue::createItem().
   *
   * Prevents duplicate requests from being queued up during the same
   * page request.
   *
   * @param $data
   * @return bool|void
   */
  public function createItem($data) {
    if (isset($data['hash'])) {
      if (in_array($data['hash'], self::$items)) {
        return;
      }
      else {
        self::$items[] = $data['hash'];
        unset($data['hash']);
      }
    }
    parent::createItem($data);
  }

  public function deleteQueue() {
    parent::deleteQueue();
    self::$items = array();
  }
}

