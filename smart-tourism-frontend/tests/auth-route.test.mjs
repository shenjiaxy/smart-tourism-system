import assert from 'node:assert/strict'
import { resolveRouteAccess } from '../src/utils/authRoute.ts'

assert.equal(
  resolveRouteAccess({ authenticated: false, role: null }, { requiresAuth: true }),
  '/login',
  'unauthenticated users should be sent to login',
)

assert.equal(
  resolveRouteAccess({ authenticated: true, role: 'user' }, { adminOnly: true }),
  '/',
  'normal users should not enter admin pages',
)

assert.equal(
  resolveRouteAccess({ authenticated: true, role: 'admin' }, { guestOnly: true }),
  '/admin',
  'logged-in admins should leave the login page for admin home',
)

assert.equal(
  resolveRouteAccess({ authenticated: true, role: 'admin' }, { adminOnly: true }),
  null,
  'admins should enter admin pages',
)

console.log('auth route guard test passed')
