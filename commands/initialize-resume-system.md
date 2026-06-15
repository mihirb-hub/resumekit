# Initialize Resume System

You are setting up a reusable, agent-agnostic resume optimization system for a user. This command is for onboarding only. Do not optimize a specific job application until initialization is complete.

## Goal

Collect as much truthful, high-signal context about the user as possible so future resume tailoring can be faster, sharper, and more complete.

This command should:

1. create the persistent workspace structure
2. ingest the user's current resume materials
3. collect preferences about positioning aggressiveness
4. require project-level context collection for important repos
5. save everything in a compact, reusable format

If initialization has already been completed and the system files are healthy, tell the user that this command only needs to be rerun when their background materially changes.

## Walkthrough Mode

This command should feel like a guided setup, not an internal checklist.

Assume the user may not be familiar with agent workflows, repo context collection, or resume-system structure.

Speak to the user like this:

1. tell them what is happening right now
2. tell them exactly what they need to do next
3. tell them where to put files
4. tell them what command to run after that
5. tell them what "done" looks like before moving on

Whenever possible, use direct instructions like:

- `Create or open one folder that will be your resume workspace.`
- `Put your current resume and any older resumes into resume-source/.`
- `If you have important project repos, you will go into those repos next and run the project-context command there.`
- `When that is done, come back here and run the main resume optimization command.`

Do not assume the user already understands the difference between onboarding, repo context collection, and job-specific tailoring. Explain that difference clearly.

---

## Workspace Structure

If missing, create:

```text
.resume-system/
  USER_PROFILE.md
  SOURCE_INVENTORY.md
  OPTIMIZATION_PREFERENCES.md
  TAILORING_LOG.md
  JAKES_RESUME_TEMPLATE.tex
  PROJECT_CONTEXT_INDEX.md
  project-context/
resume-source/
job-postings/
output/
```

---

## Step 1: Explain the Onboarding Flow

Tell the user, plainly:

- this setup only needs to happen once
- the purpose is to build a reusable context layer
- future resume optimization runs will be faster because they will reuse this saved context
- the strongest resumes usually come from more than just the current resume file
- project repos often contain missing technical detail that should be harvested

Use framing like:

`To sell you well, I need more than your current resume bullets. I want your base resume, supporting docs, and repo-level context from the projects you actually worked on.`

Then give the user a literal setup walkthrough in this order:

1. `Step 1: Put your resume files into this workspace.`
2. `Step 2: I will extract your base profile from those files.`
3. `Step 3: You will run a separate repo-context command inside important project repos so I can understand the real stack and architecture.`
4. `Step 4: After that, future resume tailoring runs will be much faster.`

If the folder is empty or nearly empty, explicitly say:

`Right now this looks like a fresh workspace. Before I can build your resume system, I need you to add your current resume and any useful supporting files.`

---

## Step 2: Ask the User for Core Materials

Tell the user to put or point you to:

- current base resume(s)
- old tailored resumes
- transcript or coursework list
- brag doc or achievement notes
- portfolio copy
- project summaries
- application answers or essays that contain useful detail

Readable file types include:

- `*.tex`
- `*.pdf`
- `*.docx`
- `*.md`
- `*.txt`

If files already exist in the workspace, use them. If not, tell the user to place them in `resume-source/`.

Use a very explicit walkthrough, like:

`Do this now:`

1. `Put your current resume into resume-source/.`
2. `If you have old tailored resumes, put those there too.`
3. `If you have a brag doc, transcript, project writeup, or portfolio notes, add those too.`
4. `Once those files are in place, rerun this initialization command if needed or let me continue scanning the folder.`

Also explain why each file helps:

- current resume: baseline wording and chronology
- old tailored resumes: what has already worked for different roles
- transcript/course list: useful when a JD asks for missing stack evidence or relevant coursework
- brag doc / notes: often contains stronger metrics than the resume itself
- project summaries / technical writeups: useful for stronger project bullets later

---

## Step 3: Ask About Positioning Aggressiveness

Ask it in a practical way, not a policy-document tone.

Use wording like:

`Pick how hard you want me to sell your background:`

- `stack_positioning = strict | aggressive | maximum-credible`
- `unlisted_but_real_skills = true | false`
- `adjacent_stack_reframing = true | false`

Then explain:

- `strict`: only use stacks explicitly shown in your current resume and source docs
- `aggressive`: surface any stack you have actually used, even if it is not currently listed well
- `maximum-credible`: aggressively map your real background to the JD using omitted-but-true skills, side projects, coursework, and repo evidence, as long as you could defend it in an interview

Allowed:

- adding real tools the user actually used but forgot to list
- surfacing class projects, side projects, research work, volunteering, or short-term work when it truthfully proves a skill
- reframing real backend, infra, frontend, data, or ML work in language closer to the JD when the underlying work really matches
- sharpening ownership language when the user genuinely drove the work

Not allowed:

- changing Python experience into Java experience if the user did not actually use Java there
- changing one framework into another framework if it did not happen
- inventing production experience for a stack that only appears in theory

Record the answer in `OPTIMIZATION_PREFERENCES.md`.

---

## Step 4: Require Repo-Level Context Collection

This is mandatory for any important project repo the user wants to sell well.

Tell the user:

`Go into one of your important project repos and run the project-context collection command. Do this for the repos you most want me to use in future resume tailoring.`

Explain why:

- resumes often underspecify the actual stack
- repos reveal libraries, frameworks, architecture, deployment, testing, tooling, and project complexity
- the user can add notes about what they personally owned so the system does not confuse team-wide tech with individual contribution

Minimum expectation:

- run the project-context command for at least 2-5 important repos if they exist
- prioritize the projects most likely to appear on the resume
- prefer repos tied to internships, jobs, startups, serious side projects, research, or shipped systems

Make the next action extremely explicit:

`What you should do next:`

1. `Pick one strong project repo.`
2. `Open that repo as your current working folder.`
3. `Run the project-context collection command there.`
4. `Repeat this for the other repos you most want me to use.`
5. `Then return to this main resume workspace.`

---

## Step 5: Build the Base User Profile

Read 2-4 of the best available resume-like sources and parse them into a structured profile.

Rules:

- extract facts from source material
- preserve ambiguity as notes if needed
- do not invent metrics
- do not invent tools
- treat repo-derived tech stack as project context, not automatic proof of personal ownership

---

## Step 6: Build the Source Inventory

Create `SOURCE_INVENTORY.md` with:

- primary resume sources
- supporting sources
- repo context sources
- evidence quality notes

Score sources by:

1. technical relevance
2. specificity
3. quantified evidence
4. recency
5. tailoring flexibility

---

## Step 7: Build Preferences

Create `OPTIMIZATION_PREFERENCES.md` with at least:

- resume length
- GPA policy
- tone
- strategy
- cover-letter default
- claims policy
- template choice
- PDF rendering default
- positioning mode
- unlisted-but-real-skills setting
- transferable-reframing setting

---

## Step 8: Create the Jake Template File

If the workspace already contains a strong Jake-style `.tex` resume, use that as the base template and record its path.

If not, use `templates/JAKES_RESUME_TEMPLATE.tex` from this repo as the default template source and copy it into `.resume-system/JAKES_RESUME_TEMPLATE.tex`.

---

## Step 9: Present the Summary and Stop

At the end of initialization, present a concise summary:

- major experiences found
- major projects found
- core skills found
- repo contexts still missing
- positioning preferences captured
- template path selected

Then tell the user exactly what to do next:

1. run the project-context collection command inside important repos
2. return to the main resume workspace
3. run the regular optimization command only after context collection is done

Use a final walkthrough-style closing like:

`Initialization is done. Your base resume system now exists.`

`Next:`

1. `Go into your strongest project repos and run the project-context command.`
2. `Come back to this workspace when you are done.`
3. `Drop a job description into job-postings/ or paste it directly.`
4. `Run the main resume optimization command to generate a tailored Jake-style .tex resume and compiled PDF.`

Do not start tailoring for a specific JD in this command.
