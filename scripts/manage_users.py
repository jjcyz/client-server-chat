#!/usr/bin/env python3
"""
User management script for the chat server database.
Allows viewing users, resetting passwords, deleting users, and creating new users.
"""

import sqlite3
import sys
import hashlib
import secrets

DB_PATH = "chat_server.db"

def hash_password(password, salt):
    """Hash password with salt using SHA-256"""
    salted = password + salt
    return hashlib.sha256(salted.encode()).hexdigest()

def generate_salt():
    """Generate a random salt"""
    return secrets.token_hex(16)

def list_users():
    """List all users in the database"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    cursor.execute("SELECT id, username, is_admin, created_at FROM users ORDER BY id")
    users = cursor.fetchall()

    if not users:
        print("No users found in database.")
        conn.close()
        return

    print("\n=== Users in Database ===")
    print(f"{'ID':<5} {'Username':<20} {'Admin':<8} {'Created At'}")
    print("-" * 60)
    for user in users:
        user_id, username, is_admin, created_at = user
        admin_status = "Yes" if is_admin else "No"
        print(f"{user_id:<5} {username:<20} {admin_status:<8} {created_at}")
    print()

    conn.close()

def reset_password(username, new_password):
    """Reset a user's password"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Check if user exists
    cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
    if not cursor.fetchone():
        print(f"Error: User '{username}' not found.")
        conn.close()
        return False

    # Generate new salt and hash password
    salt = generate_salt()
    password_hash = hash_password(new_password, salt)

    # Update user
    cursor.execute(
        "UPDATE users SET password_hash = ?, salt = ? WHERE username = ?",
        (password_hash, salt, username)
    )
    conn.commit()
    conn.close()

    print(f"Password reset successfully for user '{username}'")
    return True

def delete_user(username):
    """Delete a user from the database"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Check if user exists
    cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
    if not cursor.fetchone():
        print(f"Error: User '{username}' not found.")
        conn.close()
        return False

    # Delete user (cascade will handle sessions and messages)
    cursor.execute("DELETE FROM users WHERE username = ?", (username,))
    conn.commit()
    conn.close()

    print(f"User '{username}' deleted successfully.")
    return True

def create_user(username, password, is_admin=False):
    """Create a new user"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Check if user already exists
    cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
    if cursor.fetchone():
        print(f"Error: User '{username}' already exists.")
        conn.close()
        return False

    # Generate salt and hash password
    salt = generate_salt()
    password_hash = hash_password(password, salt)

    # Insert user
    cursor.execute(
        "INSERT INTO users (username, password_hash, salt, is_admin) VALUES (?, ?, ?, ?)",
        (username, password_hash, salt, 1 if is_admin else 0)
    )
    conn.commit()
    conn.close()

    admin_text = " (admin)" if is_admin else ""
    print(f"User '{username}' created successfully{admin_text}.")
    return True

def set_admin(username, is_admin):
    """Set or remove admin status"""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # Check if user exists
    cursor.execute("SELECT id FROM users WHERE username = ?", (username,))
    if not cursor.fetchone():
        print(f"Error: User '{username}' not found.")
        conn.close()
        return False

    cursor.execute(
        "UPDATE users SET is_admin = ? WHERE username = ?",
        (1 if is_admin else 0, username)
    )
    conn.commit()
    conn.close()

    status = "granted" if is_admin else "removed"
    print(f"Admin status {status} for user '{username}'.")
    return True

def main():
    if len(sys.argv) < 2:
        print("""
Chat Server User Management Tool

Usage:
  python3 manage_users.py list                    - List all users
  python3 manage_users.py reset <user> <pass>   - Reset user password
  python3 manage_users.py delete <user>          - Delete a user
  python3 manage_users.py create <user> <pass>   - Create a new user
  python3 manage_users.py admin <user>            - Grant admin to user
  python3 manage_users.py unadmin <user>          - Remove admin from user
  python3 manage_users.py clear                  - Delete all users (WARNING!)
        """)
        sys.exit(1)

    command = sys.argv[1].lower()

    try:
        if command == "list":
            list_users()

        elif command == "reset":
            if len(sys.argv) < 4:
                print("Error: Usage: reset <username> <new_password>")
                sys.exit(1)
            reset_password(sys.argv[2], sys.argv[3])

        elif command == "delete":
            if len(sys.argv) < 3:
                print("Error: Usage: delete <username>")
                sys.exit(1)
            confirm = input(f"Are you sure you want to delete user '{sys.argv[2]}'? (yes/no): ")
            if confirm.lower() == "yes":
                delete_user(sys.argv[2])
            else:
                print("Cancelled.")

        elif command == "create":
            if len(sys.argv) < 4:
                print("Error: Usage: create <username> <password>")
                sys.exit(1)
            is_admin = input("Make this user an admin? (yes/no): ").lower() == "yes"
            create_user(sys.argv[2], sys.argv[3], is_admin)

        elif command == "admin":
            if len(sys.argv) < 3:
                print("Error: Usage: admin <username>")
                sys.exit(1)
            set_admin(sys.argv[2], True)

        elif command == "unadmin":
            if len(sys.argv) < 3:
                print("Error: Usage: unadmin <username>")
                sys.exit(1)
            set_admin(sys.argv[2], False)

        elif command == "clear":
            confirm = input("WARNING: This will delete ALL users. Type 'DELETE ALL' to confirm: ")
            if confirm == "DELETE ALL":
                conn = sqlite3.connect(DB_PATH)
                cursor = conn.cursor()
                cursor.execute("DELETE FROM users")
                cursor.execute("DELETE FROM sessions")
                conn.commit()
                conn.close()
                print("All users deleted.")
            else:
                print("Cancelled.")

        else:
            print(f"Unknown command: {command}")
            sys.exit(1)

    except sqlite3.Error as e:
        print(f"Database error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
