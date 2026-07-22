const { PubSub } = require('@google-cloud/pubsub');
const logger = require('./logger');

/**
 * WHY GCP PUB/SUB:
 * At-least-once message delivery — even if the worker crashes,
 * the message stays in the subscription until acknowledged.
 * This guarantees no task is silently dropped.
 *
 * HOW IT WORKS:
 * Publisher (API/C++ engine) → Topic → Subscription → Worker(s)
 * Multiple workers can subscribe to the same topic for parallel processing.
 *
 * WHY NOT DIRECT HTTP CALL TO WORKER:
 * If the worker is down, the direct HTTP call fails and the task is lost.
 * Pub/Sub buffers the message until the worker is back up.
 */
const pubSubClient = new PubSub({
  projectId: process.env.GCP_PROJECT_ID
});

const TOPIC_NAME = process.env.GCP_PUBSUB_TOPIC || 'task-scheduler-topic';
const SUBSCRIPTION_NAME = process.env.GCP_PUBSUB_SUBSCRIPTION || 'task-scheduler-subscription';

async function publishTask(taskData) {
  const topic = pubSubClient.topic(TOPIC_NAME);
  const messageBuffer = Buffer.from(JSON.stringify(taskData));

  // WHY ATTRIBUTES: Pub/Sub message attributes allow filtering.
  // Workers can subscribe to only 'channel=email' tasks etc.
  const messageId = await topic.publishMessage({
    data: messageBuffer,
    attributes: {
      taskId: taskData.taskId,
      executionId: taskData.executionId,  // Idempotency key
      strategy: taskData.strategy
    }
  });

  logger.info(`Task published to Pub/Sub: ${messageId}`);
  return messageId;
}

async function createTopicIfNotExists() {
  const [topics] = await pubSubClient.getTopics();
  const exists = topics.some(t => t.name.endsWith(TOPIC_NAME));
  if (!exists) {
    await pubSubClient.createTopic(TOPIC_NAME);
    logger.info(`Created Pub/Sub topic: ${TOPIC_NAME}`);
  }
}

module.exports = { pubSubClient, publishTask, createTopicIfNotExists, TOPIC_NAME, SUBSCRIPTION_NAME };
