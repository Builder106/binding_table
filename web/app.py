from flask import Flask, request, render_template, jsonify
import subprocess
import os


def create_app():
    app = Flask(__name__, template_folder="templates", static_folder="static")

    @app.get("/")
    def index():
        return render_template("index.html")

    @app.post("/run")
    def run_code():
        code = request.form.get("code", "")
        repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
        br_path = os.path.join(repo_root, "br")
        # Ensure the binary is built and runner is executable
        try:
            subprocess.run(["chmod", "+x", br_path], check=False)
        except Exception:
            pass
        proc = subprocess.run(
            [br_path],
            input=code.encode("utf-8"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            cwd=repo_root,
        )
        output = proc.stdout.decode("utf-8", errors="ignore")
        err = proc.stderr.decode("utf-8", errors="ignore")
        return jsonify({"ok": proc.returncode == 0, "stdout": output, "stderr": err})

    return app


app = create_app()

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)


