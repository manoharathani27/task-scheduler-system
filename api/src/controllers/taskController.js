const { Task, ExecutionLog } = require('../models');
const { publishTask } = require('../config/pubsub');
const { v4: uuidv4 } = require('uuid');
const logger = require('../config/logger');

/**
 * WHY CONTROLLER LAYER:
 * Controllers handle HTTP concerns (parsing body, sending response).
 * Business logic lives in services (if complex) or directly here for simple CRUD.
 * Separation makes unit testing easier — test logic without HTTP.
 */

async function createTask(req, res) {
  try {
    const { name, cronExpression, callbackUrl, payload, executionStrategy, maxRetries } = req.body;

    if (!name || !cronExpression) {
      return res.status(400).json({ success: false, message: 'name and cronExpression are required' });
    }

    if (executionStrategy === 'HTTP_CALLBACK' && !callbackUrl) {
      return res.status(400).json({ success: false, message: 'callbackUrl required for HTTP_CALLBACK strategy' });
    }

    const task = await Task.create({
      name,
      cronExpression,
      callbackUrl,
      payload: payload || {},
      executionStrategy: executionStrategy || 'PUBSUB',
      maxRetries: maxRetries || 3,
      createdBy: req.user.id
    });

    // Publish to Pub/Sub so C++ engine picks it up
    // WHY: The C++ engine subscribes to this topic and adds
    // the task to its in-memory min-heap scheduler
    await publishTask({
      action: 'REGISTER',
      taskId: task.id,
      name: task.name,
      cronExpression: task.cronExpression,
      callbackUrl: task.callbackUrl,
      payload: task.payload,
      strategy: task.executionStrategy,
      executionId: `${task.id}_register`,
      maxRetries: task.maxRetries
    });

    logger.info(`Task created: ${task.id}`);
    res.status(201).json({ success: true, task });
  } catch (err) {
    logger.error(err);
    res.status(500).json({ success: false, message: err.message });
  }
}

async function getAllTasks(req, res) {
  try {
    const { status, page = 1, limit = 20 } = req.query;
    const where = { createdBy: req.user.id };
    if (status) where.status = status;
    const offset = (page - 1) * limit;
    const { rows, count } = await Task.findAndCountAll({
      where, order: [['createdAt', 'DESC']], limit: parseInt(limit), offset
    });
    res.json({ success: true, tasks: rows, total: count, page: parseInt(page) });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function getTask(req, res) {
  try {
    const task = await Task.findOne({
      where: { id: req.params.id, createdBy: req.user.id },
      include: [{ model: ExecutionLog, as: 'executions', limit: 10, order: [['createdAt', 'DESC']] }]
    });
    if (!task) return res.status(404).json({ success: false, message: 'Task not found' });
    res.json({ success: true, task });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function updateTask(req, res) {
  try {
    const task = await Task.findOne({ where: { id: req.params.id, createdBy: req.user.id } });
    if (!task) return res.status(404).json({ success: false, message: 'Task not found' });
    await task.update(req.body);

    // Notify C++ engine of the change
    await publishTask({ action: 'UPDATE', taskId: task.id, ...req.body, executionId: `${task.id}_update_${Date.now()}` });
    res.json({ success: true, task });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function pauseTask(req, res) {
  try {
    const task = await Task.findOne({ where: { id: req.params.id, createdBy: req.user.id } });
    if (!task) return res.status(404).json({ success: false, message: 'Task not found' });
    await task.update({ status: 'PAUSED' });
    await publishTask({ action: 'PAUSE', taskId: task.id, executionId: `${task.id}_pause_${Date.now()}` });
    res.json({ success: true, message: 'Task paused', task });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function resumeTask(req, res) {
  try {
    const task = await Task.findOne({ where: { id: req.params.id, createdBy: req.user.id } });
    if (!task) return res.status(404).json({ success: false, message: 'Task not found' });
    await task.update({ status: 'PENDING' });
    await publishTask({ action: 'RESUME', taskId: task.id, executionId: `${task.id}_resume_${Date.now()}` });
    res.json({ success: true, message: 'Task resumed', task });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function deleteTask(req, res) {
  try {
    const task = await Task.findOne({ where: { id: req.params.id, createdBy: req.user.id } });
    if (!task) return res.status(404).json({ success: false, message: 'Task not found' });
    await publishTask({ action: 'DELETE', taskId: task.id, executionId: `${task.id}_delete_${Date.now()}` });
    await task.destroy();
    res.json({ success: true, message: 'Task deleted' });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

module.exports = { createTask, getAllTasks, getTask, updateTask, pauseTask, resumeTask, deleteTask };
