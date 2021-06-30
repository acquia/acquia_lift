<?php

namespace Drupal\acquia_perz\Form;

use Drupal\Core\Form\FormBase;
use Drupal\Core\Form\FormStateInterface;
use Symfony\Component\DependencyInjection\ContainerInterface;
use Drupal\Core\Queue\QueueFactory;
use Drupal\Core\CronInterface;
use Drupal\Core\Database\Connection;
use Drupal\acquia_perz\ExportQueue;
use Drupal\Core\Entity\EntityTypeManagerInterface;

/**
 * Defines the form to export content via Queue.
 */
class ExportForm extends FormBase {

  /**
   * The entity type manager.
   *
   * @var \Drupal\Core\Entity\EntityTypeManagerInterface
   */
  protected $entityTypeManager;

  /**
   * The Export Queue Service.
   *
   * @var \Drupal\acquia_perz\ExportQueue
   */
  protected $exportQueue;

  /**
   * The queue object.
   *
   * @var \Drupal\Core\Queue\QueueFactory
   */
  protected $queueFactory;

  /**
   * The database object.
   *
   * @var \Drupal\Core\Database\Connection
   */
  protected $database;

  /**
   * The CronInterface object.
   *
   * @var \Drupal\Core\CronInterface
   */
  protected $cron;

  /**
   * Constructor.
   * @param \Drupal\Core\Entity\EntityTypeManagerInterface $entity_type_manager
   *   Entity type manager service.
   * @param \Drupal\acquia_perz\ExportQueue $export_queue
   *   Export Queue service.
   * @param \Drupal\Core\Queue\QueueFactory $queue_factory
   *   Queue factory service to get new/existing queues for use.
   * @param \Drupal\Core\Database\Connection $database
   *   The database connection to be used.
   * @param Drupal\Core\CronInterface $cron
   *   The cron service.
   */
  public function __construct(EntityTypeManagerInterface $entity_type_manager, ExportQueue $export_queue, QueueFactory $queue_factory, Connection $database, CronInterface $cron) {
    $this->entityTypeManager = $entity_type_manager;
    $this->exportQueue = $export_queue;
    $this->queueFactory = $queue_factory;
    $this->database = $database;
    $this->cron = $cron;
  }

  /**
   * {@inheritdoc}
   */
  public static function create(ContainerInterface $container) {
    $form = new static(
      $container->get('entity_type.manager'),
      $container->get('acquia_perz.export_queue'),
      $container->get('queue'),
      $container->get('database'),
      $container->get('cron')
    );
    $form->setMessenger($container->get('messenger'));
    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function getFormId() {
    return 'acquia_perz_export';
  }

  /**
   * {@inheritdoc}
   */
  public function buildForm(array $form, FormStateInterface $form_state) {
    $form['ping_cis'] = [
      '#type' => 'submit',
      '#value' => t('Ping CIS'),
      '#description' => t('Ping CIS to make an import of all entities from drupal site.')
    ];

    return $form;
  }

  /**
   * {@inheritdoc}
   */
  public function submitForm(array &$form, FormStateInterface $form_state) {
    $triggered_button = $form_state->getTriggeringElement()['#parents'][0];
    if ($triggered_button === 'ping_cis') {

    }
  }

}
