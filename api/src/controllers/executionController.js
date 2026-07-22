const { ExecutionLog, Task } = require('../models');

async function getExecutions(req, res) {
  try {
    const { taskId, status, page = 1, limit = 20 } = req.query;
    const where = {};
    if (taskId) where.taskId = taskId;
    if (status) where.status = status;
    const offset = (page - 1) * limit;
    const { rows, count } = await ExecutionLog.findAndCountAll({
      where, order: [['startedAt', 'DESC']], limit: parseInt(limit), offset,
      include: [{ model: Task, attributes: ['name', 'cronExpression'] }]
    });
    res.json({ success: true, executions: rows, total: count });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

async function getExecutionById(req, res) {
  try {
    const exec = await ExecutionLog.findByPk(req.params.id, {
      include: [{ model: Task, attributes: ['name', 'cronExpression', 'callbackUrl'] }]
    });
    if (!exec) return res.status(404).json({ success: false, message: 'Execution not found' });
    res.json({ success: true, execution: exec });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

// Called by worker after task execution (webhook from worker service)
async function recordExecution(req, res) {
  try {
    const { taskId, executionId, status, durationMs, errorMessage, attempt } = req.body;

    // WHY UPSERT: Worker might report the same executionId twice (network retry).
    // Upsert ensures we don't create duplicate records — idempotency in action.
    const [log, created] = await ExecutionLog.findOrCreate({
      where: { executionId },
      defaults: { taskId, executionId, status, durationMs, errorMessage, attempt, completedAt: new Date() }
    });

    if (!created) {
      await log.update({ status, durationMs, errorMessage, completedAt: new Date() });
    }

    // Update parent task's lastRunTime
    await Task.update({ lastRunTime: new Date() }, { where: { id: taskId } });

    res.json({ success: true, log });
  } catch (err) {
    res.status(500).json({ success: false, message: err.message });
  }
}

module.exports = { getExecutions, getExecutionById, recordExecution };
