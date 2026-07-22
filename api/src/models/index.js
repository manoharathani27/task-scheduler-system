const { DataTypes } = require('sequelize');
const { sequelize } = require('../config/database');

/**
 * WHY THESE FIELDS:
 * - cronExpression: the schedule (parsed by C++ engine)
 * - callbackUrl: where HTTP strategy sends POST requests
 * - executionStrategy: which strategy the C++ engine uses
 * - nextRunTime: cached so the API can show it without re-parsing cron
 * - lastRunTime: for analytics and debugging
 */
const Task = sequelize.define('Task', {
  id: { type: DataTypes.UUID, defaultValue: DataTypes.UUIDV4, primaryKey: true },
  name: { type: DataTypes.STRING, allowNull: false },
  cronExpression: { type: DataTypes.STRING, allowNull: false },
  callbackUrl: { type: DataTypes.STRING, allowNull: true },
  payload: { type: DataTypes.JSONB, defaultValue: {} },
  executionStrategy: {
    type: DataTypes.ENUM('HTTP_CALLBACK', 'PUBSUB'),
    defaultValue: 'PUBSUB'
  },
  status: {
    type: DataTypes.ENUM('PENDING', 'RUNNING', 'PAUSED', 'FAILED'),
    defaultValue: 'PENDING'
  },
  nextRunTime: { type: DataTypes.DATE, allowNull: true },
  lastRunTime: { type: DataTypes.DATE, allowNull: true },
  maxRetries: { type: DataTypes.INTEGER, defaultValue: 3 },
  retryCount: { type: DataTypes.INTEGER, defaultValue: 0 },
  createdBy: { type: DataTypes.UUID, allowNull: false }
}, {
  timestamps: true,
  indexes: [{ fields: ['createdBy'] }, { fields: ['status'] }]
});

/**
 * WHY EXECUTION LOG:
 * Every time a task fires, we record it.
 * This enables:
 * - Debugging failed runs
 * - Analytics (how often does this task fail?)
 * - Idempotency checks (has executionId already been processed?)
 */
const ExecutionLog = sequelize.define('ExecutionLog', {
  id: { type: DataTypes.UUID, defaultValue: DataTypes.UUIDV4, primaryKey: true },
  taskId: { type: DataTypes.UUID, allowNull: false },
  executionId: { type: DataTypes.STRING, allowNull: false, unique: true }, // Idempotency key
  status: {
    type: DataTypes.ENUM('STARTED', 'SUCCESS', 'FAILED', 'SKIPPED'),
    defaultValue: 'STARTED'
  },
  startedAt: { type: DataTypes.DATE, allowNull: false, defaultValue: DataTypes.NOW },
  completedAt: { type: DataTypes.DATE, allowNull: true },
  durationMs: { type: DataTypes.INTEGER, allowNull: true },
  errorMessage: { type: DataTypes.TEXT, allowNull: true },
  attempt: { type: DataTypes.INTEGER, defaultValue: 1 }
}, {
  timestamps: true,
  indexes: [
    { fields: ['taskId'] },
    { fields: ['executionId'], unique: true },
    { fields: ['status'] }
  ]
});

// Associations
Task.hasMany(ExecutionLog, { foreignKey: 'taskId', as: 'executions' });
ExecutionLog.belongsTo(Task, { foreignKey: 'taskId' });

const User = sequelize.define('User', {
  id: { type: DataTypes.UUID, defaultValue: DataTypes.UUIDV4, primaryKey: true },
  name: { type: DataTypes.STRING, allowNull: false },
  email: { type: DataTypes.STRING, allowNull: false, unique: true },
  password: { type: DataTypes.STRING, allowNull: false }
}, { timestamps: true });

module.exports = { Task, ExecutionLog, User };
