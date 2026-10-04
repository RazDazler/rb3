"""Small local-model/GPU smoke check; does not edit any game source."""
import argparse
import json
import time
from local_decomp_agent import MODEL, STATE, SCHEMA, SYSTEM, api, validate_proposal
from decomp_runner import write_json


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--endpoint', default='http://127.0.0.1:11435')
    parser.add_argument('--model', default=MODEL)
    args = parser.parse_args()
    prompt = '''Synthetic smoke check, not a Rock Band function. int Increment(int x)
on PowerPC has original instructions addi r3,r3,1; blr. x is in r3 and int return
is in r3. Produce the simplest C++98 function body for exactly these instructions
and explain it. Return JSON body and reason. Do not provide the signature.
The body string MUST include its outer curly braces, for example:
{"body":"{ return expression; }","reason":"Explanation"}.'''
    results = []
    for context in (4096, 6144):
        start = time.time()
        response = api(args.endpoint, '/api/generate', {
            'model': args.model, 'system': SYSTEM, 'prompt': prompt,
            'format': SCHEMA, 'stream': False, 'keep_alive': '10m',
            'options': {'num_ctx': context, 'num_predict': 160, 'temperature': 0, 'seed': 1}}, timeout=180)
        write_json(STATE / f'benchmark-{context}-response.json', response)
        parsed = json.loads(response['response'])
        try:
            body = validate_proposal(parsed, '{ return x + 1; }')
            policy_error = None
        except ValueError as error:
            body, policy_error = '', str(error)
        results.append({'context': context, 'elapsed_seconds': round(time.time() - start, 2),
                        'response': parsed, 'policy_passed': bool(body), 'policy_error': policy_error,
                        'output_tokens': response.get('eval_count'),
                        'tokens_per_second': round(response.get('eval_count', 0) / max(response.get('eval_duration', 1) / 1e9, 0.001), 2),
                        'running_model': api(args.endpoint, '/api/ps')})
    result = {'time': time.time(), 'model': args.model, 'checks': results,
              'limitation': 'Tiny synthetic inference/policy smoke test, not decompilation accuracy or full-context capacity benchmark.'}
    write_json(STATE / 'benchmark.json', result)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
