/**
 * Acquia Lift goals processing queue functions.
 */

(function ($, Drupal) {
  "use strict";

  Drupal.acquiaLiftUtility = Drupal.acquiaLiftUtility || {};
  Drupal.acquiaLiftUtility.GoalQueue = Drupal.acquiaLiftUtility.GoalQueue || (function($) {

    var acquiaLiftAPI;

    /**
     * Converts the data for a goal into the format for saving in the queue.
     *
     * @param goal
     *   An object with the following keys:
     *   - agentName: The name of the agent for this gaol
     *   - options: An object of goal options to be sent with the goal.
     * @return object
     *   A simple object of data to be saved.
     */
    function convertGoalToQueueData(goal) {
      return {'a': goal.agentName, 'o': goal.options};
    }

    /**
     * Converts the queue data into the data for a goal.
     *
     * @param item
     *   The queue item data.
     * @return object
     *   An object with the following keys:
     *   - agentName: The name of the agent for this gaol
     *   - options: An object of goal options to be sent with the goal.
     */
    function convertQueueDataToGoal(item) {
      return {
        'agentName': item.a,
        'options': item.o
      };
    }

    /**
     * Processes a goal QueueItem through the Acquia Lift service.
     *
     * @param queueItem
     *   The item to process.
     * @return boolean
     *   True if successful, false if error.
     */
    function processGoalItem(queueItem) {
      if (!acquiaLiftAPI) {
        return false;
      }
      var goal = convertQueueDataToGoal(queueItem.getData());
      acquiaLiftAPI.goal(goal.agentName, goal.options, function(response, textStatus, jqXHR) {
        return response;
      });
    }

    return {
      /**
       * Initialize the goal queue.
       *
       * @param api
       *   An instance of the Acquia Lift API.
       */
      'initialize': function (api) {
        acquiaLiftAPI = api;
      },
      /**
       * Adds goal data to the persistent queue.
       *
       * @param agentName
       *   The name of the agent for the goal.
       * @param options
       *   Goal options to send with the goal.
       */
      'addGoal': function (agentName, options) {
        var data = convertGoalToQueueData({'agentName': agentName, 'options': options});
        // Add the data to the persistent queue.
        Drupal.acquiaLiftUtility.Queue.add(data);
        // Now attempt to process the queue.
        processQueue();
      },

      /**
       * Process the queue by sending goals to the Acquia Lift agent.
       *
       * @param reset
       *   True if the queue should be reset such that all items are tried
       *   (such as in an initial processing for the page request).
       */
      'processQueue': function (reset) {
        reset = reset || false;
        // The processing status should be reset upon the initial page load.
        if (reset) {
          Drupal.acquiaLiftUtility.Queue.reset();
        }
        var item = Drupal.acquiaLiftUtility.Queue.getNext();
        while (item) {
          var success = processGoalItem(item);
          if (success) {
            // Only remove from the queue if processing was successful.
            Drupal.acquiaLiftUtility.Queue.remove(item);
          } else {
            // Otherwise mark it for re-processing.
            Drupal.acquiaLiftUtility.Queue.addBack(item);
          }
          item = Drupal.acquiaLiftUtility.Queue.getNext();
        }

      }
    }
  }($));


}(jQuery, Drupal));
