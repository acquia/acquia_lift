<?php

namespace Drupal\acquia_perz_push\Commands;

use Drush\Commands\DrushCommands;
use Drupal\acquia_perz_push\ExportQueue;

/**
 * Class QueueCommands.
 *
 * @package Drupal\acquia_perz_push\Commands
 */
class QueueCommands extends DrushCommands {

  /**
   * The export queue service.
   *
   * @var \Drupal\acquia_perz_push\ExportQueue
   */
  protected $exportQueue;

  /**
   * QueueCommands constructor.
   *
   * @param \Drupal\acquia_perz_push\ExportQueue $export_queue
   *   The export queue.
   */
  public function __construct(ExportQueue $export_queue) {
    $this->exportQueue = $export_queue;
  }

  /**
   * Rescan content.
   *
   * @command acquia:perz-rescan-content
   * @aliases ap-rc
   */
  public function rescanContent() {
    $this->exportQueue->rescanContentBulk();
    $this->output->writeln(dt("The content has been rescanned and added to the queue."));
  }

  /**
   * Purge a queue.
   *
   * @command acquia:perz-purge-queue
   * @aliases ap-pq
   */
  public function purgeQueue() {
    $this->exportQueue->purgeQueue();
    $this->output->writeln(dt("The queue has been purged."));
  }

  /**
   * Return count of queue items.
   *
   * @command acquia:perz-queue-items
   * @aliases ap-qi
   */
  public function queueItems() {
    $queue_count = intval($this->exportQueue->getQueueCount());
    $this->output->writeln(dt("The number of items in the queue @queue_count.",
      ['@queue_count' => $queue_count]));
  }

  /**
   * Export content.
   *
   * @command acquia:perz-export-content
   * @aliases ap-ec
   */
  public function exportContent() {
    $this->exportQueue->exportBulkQueueItems();
    $this->output->writeln(dt("All content has been exported from the Queue."));
  }

}
