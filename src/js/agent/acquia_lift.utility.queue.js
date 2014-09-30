/**
 * Utility functions for stateless queue cookie processing.
 *
 * Functionality includes basic read/write functionality.
 */
(function ($, Drupal) {
  "use strict";

  Drupal.acquiaLiftUtility = Drupal.acquiaLiftUtility || {};

  Drupal.acquiaLiftUtility.QueueItem = Drupal.acquiaLiftUtility.QueueItem || (function($) {
    var queueItemUid,
      queueItemData,
      queueItemProcessing = false;

    /**
     * Generates a unique ID for this queue item.
     *
     * @return
     *   An id to use that is unique across the browser instance.
     */
    function generateUniqueId() {
      return 'acquia-lift-ts-' + $.now();
    }

    /**
     * Reads the QueueItem from a simple object.
     *
     * @param params
     *   Either the data for this queue item or an object with keys:
     *   - id:  The unique id for this item
     *   - data: The data for this item
     *   - pflag: The processing flag for this item.
     */
    function fromObject(params) {
      queueItemUid = params.id ? params.id : generateUniqueId();
      queueItemData = params.data ? params.data : data;
      queueItemProcessing = params.pflag ? params.pflag : false;
    }

    return {
      /**
       * Initialize a new QueueItem from initial parameters.
       *
       * @param params
       *   Either the data for this queue item or an object with keys:
       *   - id:  The unique id for this item
       *   - data: The data for this item
       *   - pflag: The processing flag for this item.
       */
      'initialize': function (params) {
        fromObject(params);
      },

      /**
       * Returns the unique ID assigned to this queue item.
       */
      'getId': function () {
        return queueItemId;
      },

      /**
       * Returns the data that is held by this queue item.
       */
      'getData': function() {
        return queueItemData;
      },

      /**
       * Deteremines if this queue item is currently processing.
       */
      'isProcessing': function () {
        return queueItemProcessing;
      },

      /**
       * Sets the current processing flag.
       */
      'setProcessing': function (isProcessing) {
        queueItemProcessing = isProcessing;
      },

      /**
       * Determines if QueueItem is equal to the current QueueItem.
       *
       * @param queueItem
       *   The item to check against.
       * @returns
       *   True if equal, false if unequal.
       */
      'equals': function (queueItem) {
        return (queueItem.getId() == queueItemUid);
      },

      /**
       * Resets the processing flag on this queue item.
       */
      'reset': function() {
        this.setProcessing(false);
      },

      /**
       * Parses the QueueItem into a simple object.
       */
      'toObject': function () {
      return {
        'id': queueItemUid,
        'data': queueItemData,
        'pflag': queueItemProcessing
      };
    }

  }
  }($));

  Drupal.acquiaLiftUtility.Queue = Drupal.acquiaLiftUtility.Queue || (function($) {
    // @todo: Would be cool if we could swap out back-ends to local storage or
    // other mechanism.

    var cookieName = 'acquiaLiftQueue';

    /**
     * Indicates if the cookie handling script handles object serialization.
     * This is not available in the jquery.cookie.js version that ships with
     * Drupal 7 but some installations use a later version.
     *
     * @return boolean
     *   True if cookie handles serialization, false if data must be manually
     *   serialized before writing.
     */
    function cookieHandlesSerialization() {
      return ($.cookie.json && $.cookie.json == true);
    }

    /**
     * Reads the queue from storage.
     *
     * @returns array
     *   An array of QueueItems.
     */
    function readQueue() {
      var queue = $.cookie(cookieName);
      var unserialized = cookieHandlesSerialization() ? queue : $.parseJSON(queue);
      return $.isArray(unserialized) ? unserialized : [];
    }

    /**
     * Returns a fully-parsed queue.
     */
    function getAll() {
      var unserialized = readQueue(), i, num = unserialized.length, queue = [];
      for (i = 0; i < num; i++) {
        queue.push(Drupal.acquiaLiftUtility.QueueItem.initialize(unserialized[i]));
      }
      return queue;
    }

    /**
     * Returns the first unprocessed QueueItem.
     */
    function getFirstUnprocessed() {
      var unserialized = readQueue(), i, num = unserialized.length, item;
      for (i = 0; i < num; i++) {
        item = Drupal.acquiaLiftUtility.QueueItem.initialize(unserialized[i]);
        if (!item.isProcessing()) {
          return item;
        }
      }
      return null;
    }

    /**
     * Find index of a QueueItem within the Queue.
     *
     * @param queue
     *   An instance of the queue to search.
     * @param item
     *   The QueueItem to find within the queue.
     * @return int
     *   The index of the item in the queue or -1 if not found.
     */
    function indexOf(queue, item) {
      var i,
        num = queue.length,
        test;
      // Only initialize as many as we have to in order to find a match.
      for (i = 0; i < num; i++) {
        test = Drupal.acquiaLiftUtility.QueueItem.initialize(queue[i]);
        if (test.equals(item)) {
          return i;
        }
      }
      return -1;
    }


    /**
     * Writes the queue to storage.
     *
     * @param array
     *   The queue as an array.
     */
    function writeQueue(queue) {
      var queueData = [], i, num = queue.length;

      // Prepare the queue by making sure all items to save are simple objects.
      for (i = 0; i < num; i++) {
        if (queue[i] instanceof Drupal.acquiaLiftUtility.QueueItem) {
          queueData.push(queue[i].toObject())
        } else {
          queueData.push(queue[i]);
        }
      }
      // Serialize if necessary.
      if (!cookieHandlesSerialization()) {
        queue = $.toJSON(queue);
      }
      // Write to the cookie.
      $.cookie(cookieName, queue);
    }

    /**
     * Adds an existing QueueItem back to the queue for re-processing.
     *
     * @todo think about and possibly implement a retry count
     */
    function addBack(queueItem) {
      var queue = readQueue();
      var index = indexAt(queue, queueItem);
      queueItem.reset();
      if (index >= 0) {
        queue.splice(index, 1, queueItem);
      } else {
        queue.push(queueItem);
      }
      writeQueue(queue);
    };

    /**
     * Publicly accessible queue methods.
     */
    return {
      /**
       * Adds a QueueItem to the queue.
       *
       * The item can be new data to add, a new QueueItem to add, or an
       * existing QueueItem to return to the queue for re-processing.
       *
       * @param data
       *   Data or a QueueItem to add to the queue.
       */
      'add': function (data) {
        if (data instanceof Drupal.acquiaLiftUtility.QueueItem) {
          addBack(data);
          return;
        }
        var queue = readQueue();
        queue.push(Drupal.acquiaLiftUtility.QueueItem.initialize(data));
        writeQueue(queue);
      },

      /**
       * Gets the next unprocessed item in the queue for processing.
       * @returns a queueItem or null;
       *   The queueItem.
       */
      'getNext': function () {
        var item = getFirstUnprocessed();
        item.setProcessing(true);

      },

      /**
       * Removes a queueItem from the processing queue.
       *
       * @param queueItem
       *   The item to remove.
       * @returns
       *   True if the item was found to remove, false if not found.
       */
      'remove': function (queueItem) {
        var queue = readQueue();
        var index = indexOf(queue, queueItem);
        if (index >= 0) {
          queue[i].splice(index, 1);
          writeQueue(queue);
          return true;
        }
        return false;
      },

      /**
       * Resets the processing status on all items in the queue.
       */
      'reset': function () {
        var i,
          queue = getAll(),
          num = queue.length;
        for (i = 0; i < num; i++) {
          queue[i].reset();
        }
        writeQueue(queue);
      }
    }
  }($));

}(jQuery, Drupal));
