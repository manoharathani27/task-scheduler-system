const { Sequelize } = require('sequelize');
const logger = require('./logger');

/**
 * WHY SEQUELIZE:
 * ORM (Object Relational Mapper) — maps PostgreSQL tables to JS classes.
 * We write JS, it generates SQL. Also handles connection pooling automatically.
 *
 * WHY CONNECTION POOL:
 * Opening a new DB connection per request is expensive (100-200ms).
 * A pool keeps 10 connections open and reuses them — requests complete in 1-5ms.
 */
const sequelize = new Sequelize(
  process.env.DB_NAME,
  process.env.DB_USER,
  process.env.DB_PASSWORD,
  {
    host: process.env.DB_HOST || 'localhost',
    port: process.env.DB_PORT || 5432,
    dialect: 'postgres',
    logging: false,
    pool: { max: 10, min: 2, acquire: 30000, idle: 10000 }
  }
);

async function connectDB() {
  await sequelize.authenticate();
  logger.info('PostgreSQL connected');
}

module.exports = { sequelize, connectDB };
